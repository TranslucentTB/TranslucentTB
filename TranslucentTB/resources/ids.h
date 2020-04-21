#pragma once

#define IDI_MAINICON                       101
#define IDI_TRAYWHITEICON                  102
#define IDI_TRAYBLACKICON                  103
#define IDR_TRAY_MENU                      104

#define ID_TYPE_MASK                       0xF000
#define ID_TYPE_RADIOS                     0x1000
#define ID_TYPE_ACTIONS                    0x2000
#define ID_TYPE_OTHERS                     0x3000

#define ID_GROUP_MASK                      0x0F00
#define ID_GROUP_DESKTOP                   0x0000
#define ID_GROUP_VISIBLE                   0x0100
#define ID_GROUP_MAXIMISED                 0x0200
#define ID_GROUP_START                     0x0300
#define ID_GROUP_CORTANA                   0x0400
#define ID_GROUP_TIMELINE                  0x0500
#define ID_GROUP_LOG                       0x0600
#define ID_GROUP_OTHER                     0x0700

#define ID_OFFSET_MASK                     0x00FF
#define ID_OFFSET_ENABLED                  0x0001
#define ID_OFFSET_COLOR                    0x0002
#define ID_OFFSET_NORMAL                   0x0000
#define ID_OFFSET_OPAQUE                   0x0001
#define ID_OFFSET_CLEAR                    0x0002
#define ID_OFFSET_BLUR                     0x0003
#define ID_OFFSET_ACRYLIC                  0x0004

#define ID_RADIOS_DESKTOP                  (ID_TYPE_RADIOS + ID_GROUP_DESKTOP)
#define ID_RADIOS_VISIBLE                  (ID_TYPE_RADIOS + ID_GROUP_VISIBLE)
#define ID_RADIOS_MAXIMISED                (ID_TYPE_RADIOS + ID_GROUP_MAXIMISED)
#define ID_RADIOS_START                    (ID_TYPE_RADIOS + ID_GROUP_START)
#define ID_RADIOS_CORTANA                  (ID_TYPE_RADIOS + ID_GROUP_CORTANA)
#define ID_RADIOS_TIMELINE                 (ID_TYPE_RADIOS + ID_GROUP_TIMELINE)
#define ID_RADIOS_LOG                      (ID_TYPE_RADIOS + ID_GROUP_LOG)

#define ID_ACTIONS_DESKTOP                 (ID_TYPE_ACTIONS + ID_GROUP_DESKTOP)
#define ID_ACTIONS_VISIBLE                 (ID_TYPE_ACTIONS + ID_GROUP_VISIBLE)
#define ID_ACTIONS_MAXIMISED               (ID_TYPE_ACTIONS + ID_GROUP_MAXIMISED)
#define ID_ACTIONS_START                   (ID_TYPE_ACTIONS + ID_GROUP_START)
#define ID_ACTIONS_CORTANA                 (ID_TYPE_ACTIONS + ID_GROUP_CORTANA)
#define ID_ACTIONS_TIMELINE                (ID_TYPE_ACTIONS + ID_GROUP_TIMELINE)
#define ID_ACTIONS_LOG                     (ID_TYPE_ACTIONS + ID_GROUP_LOG)
#define ID_ACTIONS_OTHER                   (ID_TYPE_ACTIONS + ID_GROUP_OTHER)

#define ID_DESKTOP_COLOR                   (ID_ACTIONS_DESKTOP + ID_OFFSET_COLOR)
#define ID_DESKTOP_NORMAL                  (ID_RADIOS_DESKTOP + ID_OFFSET_NORMAL)
#define ID_DESKTOP_OPAQUE                  (ID_RADIOS_DESKTOP + ID_OFFSET_OPAQUE)
#define ID_DESKTOP_CLEAR                   (ID_RADIOS_DESKTOP + ID_OFFSET_CLEAR)
#define ID_DESKTOP_BLUR                    (ID_RADIOS_DESKTOP + ID_OFFSET_BLUR)
#define ID_DESKTOP_ACRYLIC                 (ID_RADIOS_DESKTOP + ID_OFFSET_ACRYLIC)

#define ID_VISIBLE_ENABLED                 (ID_ACTIONS_VISIBLE + ID_OFFSET_ENABLED)
#define ID_VISIBLE_COLOR                   (ID_ACTIONS_VISIBLE + ID_OFFSET_COLOR)
#define ID_VISIBLE_NORMAL                  (ID_RADIOS_VISIBLE + ID_OFFSET_NORMAL)
#define ID_VISIBLE_OPAQUE                  (ID_RADIOS_VISIBLE + ID_OFFSET_OPAQUE)
#define ID_VISIBLE_CLEAR                   (ID_RADIOS_VISIBLE + ID_OFFSET_CLEAR)
#define ID_VISIBLE_BLUR                    (ID_RADIOS_VISIBLE + ID_OFFSET_BLUR)
#define ID_VISIBLE_ACRYLIC                 (ID_RADIOS_VISIBLE + ID_OFFSET_ACRYLIC)

#define ID_MAXIMISED_ENABLED               (ID_ACTIONS_MAXIMISED + ID_OFFSET_ENABLED)
#define ID_MAXIMISED_COLOR                 (ID_ACTIONS_MAXIMISED + ID_OFFSET_COLOR)
#define ID_MAXIMISED_NORMAL                (ID_RADIOS_MAXIMISED + ID_OFFSET_NORMAL)
#define ID_MAXIMISED_OPAQUE                (ID_RADIOS_MAXIMISED + ID_OFFSET_OPAQUE)
#define ID_MAXIMISED_CLEAR                 (ID_RADIOS_MAXIMISED + ID_OFFSET_CLEAR)
#define ID_MAXIMISED_BLUR                  (ID_RADIOS_MAXIMISED + ID_OFFSET_BLUR)
#define ID_MAXIMISED_ACRYLIC               (ID_RADIOS_MAXIMISED + ID_OFFSET_ACRYLIC)

#define ID_START_ENABLED                   (ID_ACTIONS_START + ID_OFFSET_ENABLED)
#define ID_START_COLOR                     (ID_ACTIONS_START + ID_OFFSET_COLOR)
#define ID_START_NORMAL                    (ID_RADIOS_START + ID_OFFSET_NORMAL)
#define ID_START_OPAQUE                    (ID_RADIOS_START + ID_OFFSET_OPAQUE)
#define ID_START_CLEAR                     (ID_RADIOS_START + ID_OFFSET_CLEAR)
#define ID_START_BLUR                      (ID_RADIOS_START + ID_OFFSET_BLUR)
#define ID_START_ACRYLIC                   (ID_RADIOS_START + ID_OFFSET_ACRYLIC)

#define ID_CORTANA_ENABLED                 (ID_ACTIONS_CORTANA + ID_OFFSET_ENABLED)
#define ID_CORTANA_COLOR                   (ID_ACTIONS_CORTANA + ID_OFFSET_COLOR)
#define ID_CORTANA_NORMAL                  (ID_RADIOS_CORTANA + ID_OFFSET_NORMAL)
#define ID_CORTANA_OPAQUE                  (ID_RADIOS_CORTANA + ID_OFFSET_OPAQUE)
#define ID_CORTANA_CLEAR                   (ID_RADIOS_CORTANA + ID_OFFSET_CLEAR)
#define ID_CORTANA_BLUR                    (ID_RADIOS_CORTANA + ID_OFFSET_BLUR)
#define ID_CORTANA_ACRYLIC                 (ID_RADIOS_CORTANA + ID_OFFSET_ACRYLIC)

#define ID_TIMELINE_ENABLED                (ID_ACTIONS_TIMELINE + ID_OFFSET_ENABLED)
#define ID_TIMELINE_COLOR                  (ID_ACTIONS_TIMELINE + ID_OFFSET_COLOR)
#define ID_TIMELINE_NORMAL                 (ID_RADIOS_TIMELINE + ID_OFFSET_NORMAL)
#define ID_TIMELINE_OPAQUE                 (ID_RADIOS_TIMELINE + ID_OFFSET_OPAQUE)
#define ID_TIMELINE_CLEAR                  (ID_RADIOS_TIMELINE + ID_OFFSET_CLEAR)
#define ID_TIMELINE_BLUR                   (ID_RADIOS_TIMELINE + ID_OFFSET_BLUR)
#define ID_TIMELINE_ACRYLIC                (ID_RADIOS_TIMELINE + ID_OFFSET_ACRYLIC)

#define ID_OPENLOG                         (ID_ACTIONS_LOG + 0x00)
#define ID_SUBMENU_LOG                     (ID_TYPE_OTHERS + ID_GROUP_LOG + 0x00)

#define ID_LOG_TRACE                       (ID_RADIOS_LOG + 0x0)
#define ID_LOG_DEBUG                       (ID_RADIOS_LOG + 0x1)
#define ID_LOG_INFO                        (ID_RADIOS_LOG + 0x2)
#define ID_LOG_WARN                        (ID_RADIOS_LOG + 0x3)
#define ID_LOG_ERR                         (ID_RADIOS_LOG + 0x4)
#define ID_LOG_CRITICAL                    (ID_RADIOS_LOG + 0x5)
#define ID_LOG_OFF                         (ID_RADIOS_LOG + 0x6)

#define ID_EDITSETTINGS                    (ID_ACTIONS_OTHER + 0x00)
#define ID_RETURNTODEFAULTSETTINGS         (ID_ACTIONS_OTHER + 0x01)
#define ID_DISABLESAVING                   (ID_ACTIONS_OTHER + 0x02)
#define ID_HIDETRAY                        (ID_ACTIONS_OTHER + 0x03)
#define ID_DUMPWORKER                      (ID_ACTIONS_OTHER + 0x04)
#define ID_RESETWORKER                     (ID_ACTIONS_OTHER + 0x05)
#define ID_ABOUT                           (ID_ACTIONS_OTHER + 0x06)
#define ID_EXITWITHOUTSAVING               (ID_ACTIONS_OTHER + 0x07)
#define ID_AUTOSTART                       (ID_ACTIONS_OTHER + 0x08)
#define ID_TIPS                            (ID_ACTIONS_OTHER + 0x09)
#define ID_EXIT                            (ID_ACTIONS_OTHER + 0x0A)

#define IDS_OPENLOG_NORMAL                 60001
#define IDS_OPENLOG_ERROR                  60002
#define IDS_OPENLOG_EMPTY                  60003
#define IDS_AUTOSTART_NORMAL               60004
#define IDS_AUTOSTART_DISABLED_TASKMGR     60005
#define IDS_AUTOSTART_DISABLED_GPEDIT      60006
#define IDS_AUTOSTART_ENABLED_GPEDIT       60007
#define IDS_AUTOSTART_NULL                 60008
#define IDS_AUTOSTART_ERROR                60009
#define IDS_AUTOSTART_UNKNOWN              60010
