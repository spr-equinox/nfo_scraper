# Nfo Scraper

一款免改名的半自动媒体刮削器

**注意**：本软件仍然处于非常前期的测试阶段，建议先使用硬链接副本进行测试

## 特点
- 没有严格的文件名/文件夹名的限制
- 允许电影和剧集混合，**统一生成剧集格式的元数据**
- 使用可自定义的正则表达式进行标题提取
- 可自定义需要忽略的路径，并生成 ```.ignore``` 文件

## 使用方法

0. 建议将媒体库按照如下格式排列（便于其他软件识别）
大体上就是套两层文件夹（剧集-季度-视频文件），以下 ```.nfo``` 和 ```.ignore``` 由软件生成

```
媒体库
├─某个系列
│  │  tvshow.nfo
│  ├─[某个制作组] Mou Ge Fan Ju [Ma10p_1080p]
│  │  │  season.nfo
│  │  │  [某个制作组] Mou Ge Fan Ju [01][Ma10p_1080p][x265_flac_aac].mkv
│  │  │  [某个制作组] Mou Ge Fan Ju [01][Ma10p_1080p][x265_flac_aac].nfo
│  │  │  [某个制作组] Mou Ge Fan Ju [01][Ma10p_1080p][x265_flac_aac].sc.ass
│  │  │  [某个制作组] Mou Ge Fan Ju [02][Ma10p_1080p][x265_flac].mkv
│  │  │  [某个制作组] Mou Ge Fan Ju [02][Ma10p_1080p][x265_flac].nfo
│  │  │  [某个制作组] Mou Ge Fan Ju [02][Ma10p_1080p][x265_flac].sc.ass
│  │  ├─CDs
│  │  │  │  .ignore
│  │  │  └─[771332] 某个CD [24bit_48kHz] (flac)
│  │  │          01. 某首歌.flac
│  │  └─SPs
│  │        .ignore
│  │        [某个制作组] Mou Ge Fan Ju [CM01][Ma10p_1080p][x265_flac].mkv
│  │        [某个制作组] Mou Ge Fan Ju [CM02][Ma10p_1080p][x265_flac].mkv
│  ├─[某个制作组][Mou Ge Fan Ju Movie][Ma10p_2160p][v2]
│  │  │  season.nfo
│  │  │  [某个制作组][Mou Ge Fan Ju Movie][Ma10p_1080p][x265_flac].chs.ass
│  │  │  [某个制作组][Mou Ge Fan Ju Movie][Ma10p_1080p][x265_flac].mkv
│  │  └─某个无关文件夹
│  │        .ignore
│  │        某些无关文件.png
│  └─Mou Ge Fan Ju S2
│         season.nfo
│         Mou Ge Fan Ju S2.mkv
│         Mou Ge Fan Ju S2.nfo
│         Mou Ge Fan Ju S2.sc.ass
└─某部电影
        A Movie-[1080p][BDRIP][x265.FLAC].chs_jpn.ass
        A Movie-[1080p][BDRIP][x265.FLAC].mkv
        A Movie-[1080p][BDRIP][x265.FLAC].nfo
        season.nfo
```
1. 打开软件目录下的 ```config.json``` （没有的[在仓库下](https://github.com/spr-equinox/nfo_scraper/raw/master/nfo_scraper/config.json)），填好 ```tmdb_api_key```，有需要的可以自定义其他内容
1. 打开软件，把媒体文件夹拖入窗口，注意不要有重复文件夹
1. 选择“下一步”，检测搜索到的文件夹是否正确。在这步可以为需要忽略的文件夹创建 ```.ignore``` 文件，全选（ ```Ctrl + A```）列表“被忽略的路径”，选择“创建 ```.ignore``` 文件”
1. 选择“下一步”，本窗口用于创建**剧集元数据**。全选，再选择“搜索所选项”。软件会按照“搜索名称”向服务器搜索，如果有错误，可以双击“搜索名称”进行更改，再选择“搜索所选项”再次搜索；也可以双击错误的条目，或者选中错误的条目，选择“手动搜索所选项”，手动指定搜索结果；除此之外，还可以手动输入 ID 和类别，再选择“更新所选项”，手动指定条目
1. 全选，选择写入所选项，这一步会创建 ```tvshow.nfo``` 
1. 选择“下一步”，本窗口用于创建**季度元数据**，季度的 ID 将会从父文件夹处继承。继承的 ID 需要手动更新季度数据，即全选并选择“更新所选项”。手动为每一个剧集指定季度。如果 ID 有误，可以用和上一步一样的方法修正
1. 全选，选择写入所选项，这一步会创建 ```season.nfo``` 
1. 选择“下一步”，本窗口用于创建**单集元数据**。软件会先自动获取每一季度的单集信息，尝试与本地的视频进行一一对应，如果数量一致，状态会被标记为“√”，否则就是“×”。此时需要我们手动调整，双击待处理的项目，将本地视频从列表中移除、添加或者把获取到的数据移除、添加，让两边的数量和顺序对应。本地视频数量与元数据数量不匹配的话将**无法写入此季度**
1. 全选，选择写入所选项，这一步会创建 ```<文件名>.nfo``` 
1. 选择“下一步”，本窗口用于为季度文件夹中无关的子文件夹创建 ```.ignore``` 文件。全选列表，选择“创建 ```.ignore``` 文件”即可完成操作
1. 选择“完成”结束刮削

## 配置文件说明

**注意**：配置文件中**不能**包含注释

```json
{
	// 视频后缀名列表，用于识别视频文件，区分大小写
	"media_extension": [
		".mkv",
		".mp4",
		".m2ts"
	],
	// 用来提取“搜索名称”的正则表达式，数字代表提取第n个子匹配（从1开始）
	"title_regex_expression": [
		["^([\\[\\(].*?[\\]\\)]\\[* *)*([^#\\[\\]\\.\\(\\)]*)+.*$", 2]
	],
	// 忽略的路径的正则表达式
	"ignore_expression": [
		"BDMV"
	],
	// tmdb api 密钥，必填，否则无法使用
	// 申请链接 https://developer.themoviedb.org/docs/getting-started
	"tmdb_api_key": "",
	// 有需要的可以开启代理
	"using_proxy": true,
	"proxy_address": "http://127.0.0.1:7890"
}
```

