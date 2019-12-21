#include <freertos/FreeRTOS.h>
#include "config.h"

ts_accessory accessories_config[] = {
    {
        "Вентиляция в душе",
        13,
        13
    },
    {
        "Вентиляция в ванной",
        17,
        29
    },
    {
        "Свет на кухне",
        5,
        26
    },
    {
        "Люстра на кухне",
        2,
        19
    },
    {
        "Свет над рабочей зоной",
        3,
        30
    },
    {
        "Люстра в гостинной",
        6,
        20
    },
//    {
//        "Подсветка в гостинной",
//        7,
//        22
//    },
    {
        "Свет в кладовой",
        14,
        2
    },
    {
        "Свет в прихожей",
        1,
        25
    },
    {
        "Свет в гардеробной",
        23,
        23
    },
    {
        "Свет возле лестницы",
        22,
        31
    },
    {
        "Свет в коридоре на первом этаже",
        0,
        27
    },
    {
        "Свет в коридоре на втором этаже",
        30,
        4
    },
    {
        "Свет в ванной",
        21,
        17
    },
    {
        "Свет зеркала в ванной",
        20,
        21
    },
    {
        "Свет в душе",
        12,
        15
    },
    {
        "Свет зеркала в душе",
        9,
        6
    },
    {
        "Люстра в спальне",
        27,
        11
    },
    {
        "Свет в спальне",
        29,
        9
    },
    {
        "Светильник левый",
        28,
        12
    },
    {
        "Светильник правый",
        25,
        8
    },
    {
        "Свет в гардеробе",
        26,
        5
    },
    {
        "Свет на терассе",
        24,
        57
    },
    {
        "Свет в коридоре на третьем этаже",
        15,
        10
    },
    {
        "Люстра в гостевой",
        11,
        7
    },
    {
        "Свет в гостевой",
        10,
        3
    },
    {
        "Люстра в детской",
        19,
        24
    },
    {
        "Свет в детской",
        18,
        28
    }
};
