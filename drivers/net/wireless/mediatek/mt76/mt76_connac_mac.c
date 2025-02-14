// SPDX-License-Identifier: ISC
/* Copyright (C) 2020 MediaTek Inc. */

#include "mt76_connac_mcu.h"

int mt76_connac_pm_wake(struct mt76_phy *phy, struct mt76_connac_pm *pm)
{
	struct mt76_dev *dev = phy->dev;

	if (!pm->enable)
		return 0;

	if (!mt76_is_mmio(dev))
		return 0;

	if (!test_bit(MT76_STATE_PM, &phy->state))
		return 0;

	if (test_bit(MT76_HW_SCANNING, &phy->state) ||
	    test_bit(MT76_HW_SCHED_SCANNING, &phy->state))
		return 0;

	if (queue_work(dev->wq, &pm->wake_work))
		reinit_completion(&pm->wake_cmpl);

	if (!wait_for_completion_timeout(&pm->wake_cmpl, 3 * HZ)) {
		ieee80211_wake_queues(phy->hw);
		return -ETIMEDOUT;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(mt76_connac_pm_wake);

void mt76_connac_power_save_sched(struct mt76_phy *phy,
				  struct mt76_connac_pm *pm)
{
	struct mt76_dev *dev = phy->dev;

	if (!mt76_is_mmio(dev))
		return;

	if (!pm->enable || !test_bit(MT76_STATE_RUNNING, &phy->state))
		return;

	pm->last_activity = jiffies;

	if (test_bit(MT76_HW_SCANNING, &phy->state) ||
	    test_bit(MT76_HW_SCHED_SCANNING, &phy->state))
		return;

	if (!test_bit(MT76_STATE_PM, &phy->state))
		queue_delayed_work(dev->wq, &pm->ps_work, pm->idle_timeout);
}
EXPORT_SYMBOL_GPL(mt76_connac_power_save_sched);

void mt76_connac_free_pending_tx_skbs(struct mt76_connac_pm *pm,
				      struct mt76_wcid *wcid)
{
	int i;

	spin_lock_bh(&pm->txq_lock);
	for (i = 0; i < IEEE80211_NUM_ACS; i++) {
		if (wcid && pm->tx_q[i].wcid != wcid)
			continue;

		dev_kfree_skb(pm->tx_q[i].skb);
		pm->tx_q[i].skb = NULL;
	}
	spin_unlock_bh(&pm->txq_lock);
}
EXPORT_SYMBOL_GPL(mt76_connac_free_pending_tx_skbs);

void mt76_connac_pm_queue_skb(struct ieee80211_hw *hw,
			      struct mt76_connac_pm *pm,
			      struct mt76_wcid *wcid,
			      struct sk_buff *skb)
{
	int qid = skb_get_queue_mapping(skb);
	struct mt76_phy *phy = hw->priv;

	spin_lock_bh(&pm->txq_lock);
	if (!pm->tx_q[qid].skb) {
		ieee80211_stop_queues(hw);
		pm->tx_q[qid].wcid = wcid;
		pm->tx_q[qid].skb = skb;
		queue_work(phy->dev->wq, &pm->wake_work);
	} else {
		dev_kfree_skb(skb);
	}
	spin_unlock_bh(&pm->txq_lock);
}
EXPORT_SYMBOL_GPL(mt76_connac_pm_queue_skb);

void mt76_connac_pm_dequeue_skbs(struct mt76_phy *phy,
				 struct mt76_connac_pm *pm)
{
	int i;

	spin_lock_bh(&pm->txq_lock);
	for (i = 0; i < IEEE80211_NUM_ACS; i++) {
		struct mt76_wcid *wcid = pm->tx_q[i].wcid;
		struct ieee80211_sta *sta = NULL;

		if (!pm->tx_q[i].skb)
			continue;

		if (wcid && wcid->sta)
			sta = container_of((void *)wcid, struct ieee80211_sta,
					   drv_priv);

		mt76_tx(phy, sta, wcid, pm->tx_q[i].skb);
		pm->tx_q[i].skb = NULL;
	}
	spin_unlock_bh(&pm->txq_lock);

	mt76_worker_schedule(&phy->dev->tx_worker);
}
EXPORT_SYMBOL_GPL(mt76_connac_pm_dequeue_skbs);

int mt76_connac_remain_on_channel(struct ieee80211_hw *hw,
				  struct ieee80211_vif *vif,
				  struct ieee80211_channel *chan,
				  struct mt76_connac_roc *roc,
				  int duration,
				  enum ieee80211_roc_type type)
{
	struct mt76_phy *phy = hw->priv;
	int err;

	if (test_and_set_bit(MT76_STATE_ROC, &phy->state))
		return 0;

	roc->grant = false;
	err = mt76_connac_mcu_set_roc(phy->dev, vif, chan, duration);
	if (err < 0) {
		clear_bit(MT76_STATE_ROC, &phy->state);
		return err;
	}

	if (!wait_event_timeout(roc->wait, roc->grant, HZ)) {
		mt76_connac_mcu_set_roc(phy->dev, vif, NULL, 0);
		clear_bit(MT76_STATE_ROC, &phy->state);
		return -ETIMEDOUT;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(mt76_connac_remain_on_channel);

void mt76_connac_cancel_remain_on_channel(struct ieee80211_hw *hw,
					  struct ieee80211_vif *vif,
					  struct mt76_connac_roc *roc)
{
	struct mt76_phy *phy = hw->priv;

	if (!test_and_clear_bit(MT76_STATE_ROC, &phy->state))
		return;

	del_timer_sync(&roc->timer);
	cancel_work_sync(&roc->work);

	roc->grant = false;
	mt76_connac_mcu_set_roc(phy->dev, vif, NULL, 0);
}
EXPORT_SYMBOL_GPL(mt76_connac_cancel_remain_on_channel);

static void mt76_connac_roc_iter(void *priv, u8 *mac,
				 struct ieee80211_vif *vif)
{
	struct mt76_dev *dev = priv;

	mt76_connac_mcu_set_roc(dev, vif, NULL, 0);
}

void mt76_connac_roc_handler(struct mt76_phy *phy,
			     struct mt76_connac_roc *roc)
{
	if (!test_and_clear_bit(MT76_STATE_ROC, &phy->state))
		return;

	roc->grant = false;
	ieee80211_iterate_active_interfaces(phy->hw,
					    IEEE80211_IFACE_ITER_RESUME_ALL,
					    mt76_connac_roc_iter, phy->dev);
	ieee80211_remain_on_channel_expired(phy->hw);
}
EXPORT_SYMBOL_GPL(mt76_connac_roc_handler);
