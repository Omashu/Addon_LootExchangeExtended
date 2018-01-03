# Addon For YorozuyaGS
https://github.com/goodwinxp/Yorozuya

# Configure `global.json`

```
{
	"name" : "addon.loot_exchange_extended",
	"config" : {
	  "activated" : true,
	  "flush_logs" : false,
	  "config_path" : "./YorozuyaGS/loot_exchange_extended.json"
	}
}
```

# Configure `loot_exchange_extended.json`

```
[
	{
		"activated" : true, // is active loot convert to money
		"id" : "ipgld01", // loot server id
		"type" : 6, // money type
		"value" : 1 // money value
	},
	...
]
```

# Money types

```
enum class e_money_type : uint8_t
{
    cp = 0,
    gold = 1,
    pvp_point = 2,
    pvp_point_2 = 3,
    processing_point = 4,
    hunter_point = 5,
    gold_point = 6,
    num
};
```