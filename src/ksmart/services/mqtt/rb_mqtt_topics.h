/*
 * rb_mqtt_topics.h — Topic + constants RelayBox dùng chung với ZoneX và backend.
 *
 * Bắt buộc đồng bộ với go/pkg/events/mqtt_topics.go của backend.
 * Local topic là phần RelayBox publish/subscribe; broker nằm trên ZoneX.
 */
#ifndef RB_MQTT_TOPICS_H
#define RB_MQTT_TOPICS_H

#define RB_ORIGIN_RB    "rb"
#define RB_ORIGIN_CORE  "core"
#define RB_ORIGIN_CLOUD "cloud"
#define RB_ORIGIN_BE    "be"

#define RB_ACTION_ON  "ON"
#define RB_ACTION_OFF "OFF"

/* Fmt strings — dùng snprintf với boxId. */
#define RB_TOPIC_FMT_HELLO      "rb/%s/hello"
#define RB_TOPIC_FMT_STATUS     "rb/%s/status"
#define RB_TOPIC_FMT_LWT        "rb/%s/lwt"
#define RB_TOPIC_FMT_CMD        "rb/%s/cmd"
#define RB_TOPIC_FMT_NAME_SET   "rb/%s/name/set"
#define RB_TOPIC_FMT_NAME_SYNC  "rb/%s/name/sync"

/* Broker mặc định trên ZoneX — RelayBox resolve qua Avahi/mDNS. */
#define RB_BROKER_DEFAULT_HOST  "KSmartZoneX.local"
#define RB_BROKER_DEFAULT_PORT  1883

/* Số kênh cố định cho RelayBox thế hệ mới (toàn bộ RS485). */
#define RB_CHANNEL_COUNT 16

#endif /* RB_MQTT_TOPICS_H */
