/* SPDX-License-Identifier: ISC */
/* Copyright (C) 2020 MediaTek Inc. */

#ifndef __MT76_CONNAC_H
#define __MT76_CONNAC_H

#include "mt76.h"

#define MT76_CONNAC_SCAN_IE_LEN			600
#define MT76_CONNAC_MAX_SCHED_SCAN_INTERVAL	10
#define MT76_CONNAC_MAX_SCHED_SCAN_SSID		10
#define MT76_CONNAC_MAX_SCAN_MATCH		16

#define MT76_CONNAC_COREDUMP_TIMEOUT		(HZ / 20)
#define MT76_CONNAC_COREDUMP_SZ			(128 * 1024)

enum {
	CMD_CBW_20MHZ = IEEE80211_STA_RX_BW_20,
	CMD_CBW_40MHZ = IEEE80211_STA_RX_BW_40,
	CMD_CBW_80MHZ = IEEE80211_STA_RX_BW_80,
	CMD_CBW_160MHZ = IEEE80211_STA_RX_BW_160,
	CMD_CBW_10MHZ,
	CMD_CBW_5MHZ,
	CMD_CBW_8080MHZ,

	CMD_HE_MCS_BW80 = 0,
	CMD_HE_MCS_BW160,
	CMD_HE_MCS_BW8080,
	CMD_HE_MCS_BW_NUM
};

enum {
	HW_BSSID_0 = 0x0,
	HW_BSSID_1,
	HW_BSSID_2,
	HW_BSSID_3,
	HW_BSSID_MAX = HW_BSSID_3,
	EXT_BSSID_START = 0x10,
	EXT_BSSID_1,
	EXT_BSSID_15 = 0x1f,
	EXT_BSSID_MAX = EXT_BSSID_15,
	REPEATER_BSSID_START = 0x20,
	REPEATER_BSSID_MAX = 0x3f,
};

struct mt76_connac_pm {
	bool enable;

	spinlock_t txq_lock;
	struct {
		struct mt76_wcid *wcid;
		struct sk_buff *skb;
	} tx_q[IEEE80211_NUM_ACS];

	struct work_struct wake_work;
	struct completion wake_cmpl;

	struct delayed_work ps_work;
	unsigned long last_activity;
	unsigned long idle_timeout;
};

struct mt76_connac_coredump {
	struct sk_buff_head msg_list;
	struct delayed_work work;
	unsigned long last_activity;
};

struct mt76_connac_roc {
	struct work_struct work;
	struct timer_list timer;
	wait_queue_head_t wait;
	bool grant;
};

extern const struct wiphy_wowlan_support mt76_connac_wowlan_support;

static inline bool is_mt7921(struct mt76_dev *dev)
{
	return mt76_chip(dev) == 0x7961;
}

static inline bool is_mt7663(struct mt76_dev *dev)
{
	return mt76_chip(dev) == 0x7663;
}

int mt76_connac_pm_wake(struct mt76_phy *phy, struct mt76_connac_pm *pm);
void mt76_connac_power_save_sched(struct mt76_phy *phy,
				  struct mt76_connac_pm *pm);
void mt76_connac_free_pending_tx_skbs(struct mt76_connac_pm *pm,
				      struct mt76_wcid *wcid);

static inline void
mt76_connac_mutex_acquire(struct mt76_dev *dev, struct mt76_connac_pm *pm)
	__acquires(&dev->mutex)
{
	mutex_lock(&dev->mutex);
	mt76_connac_pm_wake(&dev->phy, pm);
}

static inline void
mt76_connac_mutex_release(struct mt76_dev *dev, struct mt76_connac_pm *pm)
	__releases(&dev->mutex)
{
	mt76_connac_power_save_sched(&dev->phy, pm);
	mutex_unlock(&dev->mutex);
}

void mt76_connac_pm_queue_skb(struct ieee80211_hw *hw,
			      struct mt76_connac_pm *pm,
			      struct mt76_wcid *wcid,
			      struct sk_buff *skb);
void mt76_connac_pm_dequeue_skbs(struct mt76_phy *phy,
				 struct mt76_connac_pm *pm);
int mt76_connac_remain_on_channel(struct ieee80211_hw *hw,
				  struct ieee80211_vif *vif,
				  struct ieee80211_channel *chan,
				  struct mt76_connac_roc *roc,
				  int duration,
				  enum ieee80211_roc_type type);
void mt76_connac_cancel_remain_on_channel(struct ieee80211_hw *hw,
					  struct ieee80211_vif *vif,
					  struct mt76_connac_roc *roc);
void mt76_connac_roc_handler(struct mt76_phy *phy,
			     struct mt76_connac_roc *roc);

#endif /* __MT76_CONNAC_H */
