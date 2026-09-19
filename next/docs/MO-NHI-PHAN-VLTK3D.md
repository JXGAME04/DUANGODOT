# Mổ nhị phân bản VLTK3D (Võ Lâm Tình Kiếm 3D — GrowX JSC, `D:\game3gTQ\VLTK3D`) — hệ thống map, NPC, kỹ năng, hình chuyển động; so với bản tham khảo 剑网江湖 và JX NEXT 3D

Ngày 2026-09-19. Bản mổ: `VLTK3D_AT2_8.exe` (installer 2,9 GB) → `D:\game3gTQ\VLTK3D\update\` (Game.exe + GameAssembly.dll 103 MB,
`Game_Data\StreamingAssets` 1 685 tệp, 3,2 GB). Công cụ và kết quả trung gian nằm ngoài git ở `D:\game3gtQ_mo\vltk3d\`:
`il2cpp_meta39.py` (đọc `global-metadata.dat` **v39**), `il2cpp_bin39.py` → `meta_vltk3d_res.txt` (21 221 lớp, 159 064 hàm, RVA
của 141 130 hàm), `disasm39.py "Lop.Ham"|rva:0x..` (capstone, chú thích chuỗi/trường/hàm gọi), `classes_vltk3d.txt` (1 784 lớp
của Assembly-CSharp), `literals_vltk3d.txt` (27 464 chuỗi), `balanghuyen_mpk.bin` (gói map Ba Lăng đã giải nén).
Mọi tên lớp/hàm dưới đây có trong `meta_vltk3d_res.txt`; số RVA là của `GameAssembly.dll` bản này.

## 0. Kết luận ngắn — khác gì

| | **VLTK3D (GrowX)** | **剑网江湖 3D** (bản tham khảo đã mổ, `MO-NHI-PHAN-3D.md`) | **JX NEXT 3D** (nhánh `exp/3d-baling`) |
|---|---|---|---|
| Nguồn luật | **Server dòng JX1** (giao thức Bishop/GameServer y hệt JX1: `WORLD_SYNC`, `NPC_SYNC`, `NPC_SKILL_SYNC`, `OBJ_MISSLE_SYNC`, `PLAYER_SCRIPTACTION_SYNC`…; mã hoá gói `KsgCodec`; script Lua `\script\jx1m\…`) | server riêng của bản 3D (bảng `scn_list`, `skill_main`, `cha_list`… của họ) | zone C++ theo `jx_linux_y` (JX1/2.0), giao thức protobuf riêng |
| Client | **client JX1 port sang C# (Unity 6000.3, IL2CPP)**: `GameWorld.KRegion/SubWorld/WorldMessageQueue`, `KMissleDataStruct`, `KSkill`, `KSkillSpecial`, `KNpcResList/KStateSpr`, `AutoFindPath` (= `KAutoFindPath`), `NpcFindPath`, `m_nPeopleIdx/m_nTargetIdx/eDoing`… + lớp hiển thị 3D | client 3D viết mới (Creature/Player/TaskMove…, AIS navmesh) | client Godot mỏng: chỉ hiển thị, luật ở zone |
| Map | **dữ liệu map JX1 là gốc**: 341 gói `.mpk` (= `.wor` + bảng chướng ngại/bẫy `[16][32]` của từng region), `map_list.ini` 1 041 map; cảnh 3D đặt khớp lên lưới JX1 bằng **phép affine hiệu chỉnh 3 cặp điểm** (`MapService.Calibrate/Solve3x3`); 41 gói nằm trong thư mục tên Việt (suy đoán = map đã có cảnh 3D) | cảnh Unity + navmesh AIS + height mesh, toạ độ mét; bảng vùng `scn_area_list` | navmesh bản tham khảo rasterize thành lưới 32 (`map.json`), NPC từ mark; 45 map |
| NPC | template JX1 (`createnpc/53_npc.txt`: 31 Kim Miêu, 42 Hươu đốm, 43 Heo trắng…) → prefab 3D theo **tên res** (`NPCResDictSO.FindByPrefabName`); trang bị theo **res id 2.0** (áo/nón/vũ khí/ngựa/phi phong/bội sức) → mesh (`MeshSODict`, `SkeletonTransplant`) | `cha_list → cha_pic`, da theo `model_list` | template JX1 → `cha_pic` bản tham khảo (`models.json`), áo theo `model_list` (3D-76), vũ khí theo `MeleeRes` |
| Kỹ năng | **KMissle chạy trên client** như 2.0 (`MissleManager/Missle/MissileCollisionJob`), hình theo **`MissleResId` của `missles.txt`** (`MissleResDictSO` + biến thể skin), `KSkillSpecial` theo `sfx`, trạng thái → VFX (`StateResDictSO`), hiệu ứng trúng theo ngũ hành (`HitElementSO`) | `skill_main/skill_childobj/sfx_object` của họ, đạn do server họ | đạn ở zone (gói `G2C_MISSLE`), hình ghép theo **id kỹ năng** (`skill_map.json`) từ prefab bản tham khảo |
| Hình chuyển động | **Mecanim**: 2 AnimatorController nam/nữ (`ACDictSO`), tham số `STATE/GATEWAY/TYPE_HORSE/SWITCH_RIDE/CHECK_PK/SKILL_PARAM/STYLE_PARAM`; clip đổi theo (giới × lớp Base/Combat/Horse × việc × **gateway** = loại vũ khí) `AnimVariantDictSO`; máy trạng thái 16 trạng thái (đánh chạy, đánh xông, combo, khinh công…); vải `MagicaCloth2`, dây `Obi` (ngựa dắt) | `Animation` legacy theo bone prefab, nhóm `anim_group` theo vũ khí, 30 fps | glTF `AnimationPlayer`, nhóm clip theo vũ khí/ngựa |
| Tài nguyên | bundle Addressables 2.8.1 bọc `TPLBND1` (AES khoá 16 byte **theo phiên từ server**, giải mã trong `UnityABPlugin.dll` bị bảo vệ) → **không đọc được** model/animation/VFX | bundle Unity giải mã được (khoá trong `khoa_bundle.txt`) | glTF/PNG/JSON của ta |

Khác biệt cốt lõi: VLTK3D giữ **toàn bộ dữ liệu và logic client JX1** (lưới chướng ngại, bẫy, đạn, AI đuổi đánh, bảng npcres, bảng
âm thanh) và chỉ thay lớp vẽ 2D bằng 3D — cảnh 3D phải "khớp" vào lưới JX1 (affine). JX NEXT làm ngược: luật ở zone, cảnh 3D của bản
tham khảo là gốc, lưới đi lại rasterize từ navmesh. Vì thế VLTK3D chạy được **mọi** map/NPC/bẫy/script JX1 ngay (chỉ thiếu cảnh 3D),
còn ta mỗi map 3D phải ghép lại NPC/bẫy/vùng. Phần 7 nêu điều nên lấy về.

## 1. Gói dựng, bảo vệ, cách đọc

- `app.info`: `GrowX JSC / Võ Lâm Tình Kiếm 3D`; `GameAssembly.dll` chứa chuỗi `6000.3.9f1` (Unity 6.3); `UnityPlayer.dll` không mang số
  bản. IL2CPP với `global-metadata.dat` **phiên bản 39** (đầu `AF 1B B1 FA 27 00 00 00`, 19,7 MB): header không còn cặp (offset,size)
  mà là **31 bộ ba (offset, size, count)**; bản ghi bị nén: `TypeDefinition` 82 byte (`genericContainerIndex` int16, bỏ
  `elementTypeIndex`), `MethodDefinition` 32 byte (`declaringType` uint16, `genericContainerIndex` int16), `Image` 36 byte
  (`typeStart/typeCount/exportedStart/exportedCount` uint16), `StringLiteral` 4 byte (chỉ `dataIndex`, độ dài = mục kế tiếp). Bố cục
  này đo trực tiếp từ byte (`il2cpp_meta39.py` ghi rõ); fork Il2CppDumper hỗ trợ v35–v39 tồn tại (Doppelglower) nhưng không cần.
  `Il2CppCodeGenModule` (tên ảnh + `methodPointers`) và `MetadataRegistration.types` tìm như bản v31 → RVA đầy đủ.
- 121 assembly: `Assembly-CSharp` (2 425 kiểu), `Network.dll` (giao thức JX1), `JXShared.dll` (`KsgCodec.DecodeEncode` = mã hoá luồng
  JX1), `PakSystem.dll` (gói `.pak` riêng: `PakHeader/PakEntry` nén + CRC32), `TPL.CustomAddressables.Runtime.dll`,
  `TPL.GameSDK.Windows.dll` (đăng nhập/thanh toán), `xlua.dll` + `XLua` (115 lớp) → logic có thể nằm trong Lua, `Obi` (dây),
  `MagicaClothV2` (vải), `spine-unity` (UI Spine), `DOTween`, `UniTask`, `Sirenix Odin`, `Firebase`, `Cinemachine`, URP
  (`Unity.RenderPipelines.Universal`, `GPUDriven`), `AmazingAssets.TerrainToMesh`, `Boxophobic.AtmosphericHeightFog`, `MTE_script`.
- Tài nguyên: Addressables 2.8.1 (`aa/catalog.bin` nhị phân, `settings.json`), 1 291 bundle trong `aa/StandaloneWindows64/`, tên nhóm
  đọc được: `balanghuyen_scenes_all` ×4, `giangtanthon_scenes_all`, `chutientran_scenes_all` (203 MB), `balanghuyen_assets_all`,
  `scenes_scenes_all` ×3, `duplicateassetisolation_assets_all` ×3, `so_mesh{ao,non,vukhi,ngua,phiphong}_temp_assets_all` (mesh áo, nón,
  vũ khí, ngựa, phi phong), `sfx_gameplay_assets_all`, `music_assets_all`, `uis_assets_all` ×3, `previewmodelframes_assets_all`, còn lại
  băm md5. **Mọi bundle bọc `TPLBND1`**: `BundleHeader {Version, Flags, KeyId u16, Iv[16]}` (`BundleFormatConstants`: IvSize 16, KeySize
  16, BlockSize) rồi mã AES; `CustomAssetStream` giải mã theo luồng (seek được → nhiều khả năng chế độ CTR), khoá 16 byte lấy **theo phiên**
  (`AssetBundleSession.SetSession(keyId, key16)` / `SetWrappedSession(keyId, wrapped32)`; `AddressableDecryptionBootstrap.SetKeyFromServer`)
  qua hàm gốc trong `UnityABPlugin.dll` (12,6 MB, đóng gói/ảo hoá: section `.IYd/.<,>/.7Lz`, chỉ export `uab_query`). ⇒ Không mở được
  model, animation, VFX bằng tĩnh; và đó là tài sản riêng của họ — bản mổ này chỉ xem **cách tổ chức**.
- Không mã hoá: `StreamingAssets/Settings/maps/` (bảng map JX1, xem §2), `Settings/npcres/*.txt` được tham chiếu (`npc动作声音表.txt`,
  `主角动作声音表.txt`, `状态图形对照表.txt` = bảng của client 2.0 mà ta đã port), `UI/loadtip.ini`, `tpl-service.json`
  (`game_code jx3d`, `secret_key` cho SDK), `feedback-service.json`, `google-services-desktop.json`.

## 2. Hệ thống map

### 2.1 Dữ liệu JX1 là gốc
- `Settings/maps/map_list.ini` (cp1258/TCVN3) = `MapList.ini` của JX1: 1 041 mục (1 002 map gốc + 39 bản sao 2xxx/3xxx), mỗi mục
  `N=<thư mục>`, `N_name`, `N_MapPos`, `N_MapType`, `N_NpcSeriesAuto`, `N_AutoGoldenNpc`, `N_GoldenType`, `N_GoldenDropRate`,
  `N_NormalDropRate` — đúng khoá `KMapSettings` mà zone của ta đọc (`\settings\droprate\…` giữ nguyên đường dẫn JX1). 1 028 mục còn
  thư mục GBK gốc (`西北南区\凤翔`…), **13 mục đã đổi sang thư mục Việt** (`field/hoason`, `cave/…`, `City/thanhdo`, `other/…`) — dấu hiệu
  map đã có cảnh 3D. `worldset_1.ini/_12.ini` = danh sách map mở theo "world" của server.
- **341 gói `.mpk`** (`MPK1`, 3,7 MB cả thảy): `MapPack.FromPackBytes` — header `MPK1, u16 ver 1, u16 1, u32 count, u32 total, u32
  compressed`, mục lục `{u16 len, tên, u32 offset, u32 size}`, phần dữ liệu **zlib raw** (deflate không header). Ba Lăng
  (`country/balanghuyen.mpk`): `.wor` (298 byte, `[MAIN] rect=74,83,117,116 … [Weather]` = tệp world JX1) + **1 496 tệp
  `<x>_<y>.dat` 4 096 byte** cho lưới region 34 × 44 (x 83..116, y 74..117): 2 048 byte `m_Obstacle[16][32]` (long, thứ tự [x][y] như
  `KRegion.h` JX1) + 2 048 byte `m_dwTrap[16][32]`; giá trị chướng ngại = loại ở 4 bit thấp (1 Normal…) | dạng ở 4 bit cao (LT/RT/LB/RB
  = nửa ô chéo, `KRegion::GetBarrier`), thấy 0x11, 0x41, 0x51, 0x21, 0x31, 0xD1… — **đúng dữ liệu `Region_S.dat` của server JX1**
  (bản của ta: `jxassets export-map` → `obstacle.bin` + `traps`). 415/1 496 region có dữ liệu.
- `createnpc/53_npc.txt` (Ba Lăng = map 53 JX1): `npctemplateid npcname posx posy level series camp script value` với **toạ độ Mps tuyệt
  đối JX1** (45815, 93147…) và template `npcs.txt` JX1 (31 Kim Miêu, 42 Hươu đốm, 43 Heo trắng — chính các template ta ghép trong
  `CHA_TO_TEMPLATE`); `53_obj.txt`: vật thể (303 ngọc bội, 6 bia đá, 295/296 dấu hiệu) với script `\script\npc\village\balang\obj\*.lua`;
  `npc_list.txt`: NPC sự kiện theo `MapId MapX MapY` + `\script\jx1m\sukien\…\npc_sudungevent.lua` + ngày bắt đầu/kết thúc. ⇒ server
  của họ chạy script Lua JX1 ("jx1m").

### 2.2 Client: JX1 core port sang C#
- `GameWorld.KRegion` (`SCENE_LOGIC_CELL_WIDTH/HEIGHT`, `_1024`, `SCENE_REGION_WIDTH/HEIGHT`, `SCENE_REGION_CELL_NUM_H/V`, danh sách
  `NpcList/ObjList/MissileList/PlayerList`, `ConnectRegion[]`, `npcRef/objRef/mslRef`), `GameWorld.SubWorld` (`Regions[]`,
  `ClientRegionIdx[]`, `RegionBeginX/Y`, `WorldRegionWidth/Height`, `MessageQueue`, `MAX_REGION`, `DIR_UP/RIGHT/DOWN/LEFT`),
  `GameWorld.WorldMessage/Queue`, `GameWorld.ObstacleType/MoveObjKind` — bản chép `KSubWorld/KRegion` của JX1.
- `MapDataService`: tải gói theo `map_list.ini` (`k_MapsRelative`, `WorEncoding`, `_maplistPaths`, `CurrentTemplateId`,
  `RegionBeginX/Y`, `MinX..MaxY`, `TotalGridWidth/Height`), `ReadPackEntry("<x>_<y>.dat")`.
- `SceneObstacleMap.Load(thư mục)` → `TryParseRegionFileName` → `ParseAndStoreRegion(x, y, bytes)` → `int[,]` mỗi region; truy vấn
  `GetObstacleInfo(mpsX, mpsY)`, `GetObstacleInfoMin(mps, subX, subY)` (= `GetBarrierMin` độ chính xác ×1024 của JX1), **dạng chéo**
  `ObstacleShape` 22 giá trị (Full, LT/RT/LB/RB, các phần tư, nửa dọc/ngang, đảo, bù) + `GetSlideNormal2D/3D` →
  `ObstacleContactSlide.ComputeSlide/GetSlideTangent/SlideRatio`: nhân vật **trượt dọc tường** khi đâm chéo (2D JX1 chỉ dừng);
  `FindNearestWalkableWorldPos`, `IsStraightLineWalkable`, `TryGetStraightLineStop`; `ObstacleMapVisualizer` vẽ lưới để soát.
- `AutoFindPath` = `KAutoFindPath` JX1 (`STATE_PASS/OBSTACLE/ISLAND`, `Pretreatment` tô đảo, `NDX8/NDY8/MOV_COST8`) + **A\*** heap
  (`MinHeap`), `SmoothPath` + `HasLineOfSight`/`IsChordClearFine` (`FINE_SAMPLE_MPS`), `CLEARANCE_STEP`; `RequestNpcBotFindPath` phục
  vụ gói `S2C_NPCBOT_PATH_REQUEST` (server nhờ client tìm đường cho "npc bot"!). `NpcFindPath` = `KNpcFindPath` JX1 (`Dir64To8`,
  `CheckBarrier`, `GetDir`, `CheckDistance`).
- **Toạ độ Mps ↔ thế giới** (`MapService`): `Mps2WorldPos(mapX, mapY, y)`: u = x/512, v = y/512 (hằng `0.00195312`) → `X = m00·u +
  m01·v + offsetX`, `Z = m10·u + m11·v + offsetZ`; `WorldPos2Mps` nghịch đảo (`EnsureInvDet`); ma trận do **`Calibrate(serverA, unityA,
  serverB, unityB, serverC, unityC)`** giải `Solve3x3` từ **3 cặp điểm** (Mps ↔ Unity) mỗi map — nghĩa là cảnh 3D được dựng/đặt tự do
  rồi hiệu chỉnh khớp lưới JX1 bằng affine (có xoay/lệch: mặc định `m00 −2.4064, m01 3.584, m10 3.8912, m11 3.328, offset
  (−225.52, −1433.84)`; chỉ là giá trị chờ hiệu chỉnh). Tầm kỹ năng hình tròn theo Mps thành **ellipse** trong thế giới
  (`_ellipseUnitMajor/Minor/YawDeg`, `GetWorldEllipseForMpsRange`, `WorldRadiusForMpsRange`, `ClampWorldTargetToMpsRange`).
  Hướng 64 (`DirToRotation`, `RotationToDir`, `GetDirIndex`, bảng `g_nSin/g_nCos` JX1), khoảng cách `GetDistance` JX1,
  `LegacyMapCoords.ToMps(x, y)`: đơn vị ô cổ điển 64×32 (`x<<6, y<<5`) hay đối xứng 32 (`<<5`) — hai cách đọc toạ độ script.
  **Tốc độ**: `SkillService.ServerSpeedToWorld(stat) = stat × 18 × MpsWorldScaleForSpeed` (18 = `ORIGINAL_GAME_FPS`, cùng 18 khung/giây
  luật của ta); `SPEED_SCALE_MULT 1.414`.
- Cảnh 3D: `MapDictSO/MapDictItem {MapID, SceneName, SceneMapName, MapType (Country/City/Capital/Cave/BattleField/Field/Others/Tong),
  MapPos, PrefabKeys[], MiniMapSize, VolumeProfile}` (khớp thư mục `.mpk`); `VietKiem.Levels.SceneLoadService` tải scene Addressable +
  các nhóm prefab (`ScenePrefabGroup`, `MaxConcurrentInstantiate`, timeout từng prefab); `MapRegionController`: chia cảnh thành
  `MapRegion` (ô vuông `_regionSquare` + `_offset`, mỗi region có `_zones[]` con, `_activeRegionLevel` vòng region quanh người chơi,
  cắt frustum `_frustumPadding`, `_zoneEnterRadius/_zoneExitRadius`, kích hoạt dần từng khung `_zoneActivateFrames`);
  `PooledMapRegionController` + `MapPoolData` (ScriptableObject: `prototypes[]`, `regions[] {regionIndex, worldCenter, groups[]
  {prototypeIndex, positions/rotations/scales}}`) = cây/cỏ pool theo region, `InstancedMapGrassRenderer/VegetationRenderer` (GPU
  instancing, `GrassBudgetGovernor`, `VegetationBudget`), `ShadowCasterCuller`, `MapGroundQuality`, `MeshSimplify*` (LOD nướng sẵn),
  `DayNightSettingController`, `WaterReflection/URPWater`, `MapPreloadService` (tải map từ CDN nền, tạm dừng/tiếp tục, `UiDownloadContent`).
- Minimap: `MiniMapController` + `CalibrationPoint` (hiệu chỉnh ảnh minimap ↔ Mps), `MapConverter.Server2MiniMap`, `UiBigMap` với
  `UIBigMapNPCSlot`.

### 2.3 So sánh
- 剑网江湖: cảnh gốc tự thiết kế, đi lại theo navmesh AIS + height mesh, vùng an toàn theo `scn_area_list` (ta đã port 3D-74/75).
- JX NEXT: `make_map3d.py` rasterize navmesh thành lưới 32 đơn vị, NPC từ mark, bẫy = lối ra; zone dùng lưới đó; toạ độ 1 đơn vị =
  0,02 m, không xoay. VLTK3D dùng nguyên lưới JX1 (ô 32×32 Mps trong region 512×1024) và affine — ưu: mọi script/bẫy/NPC JX1 chạy ngay;
  nhược: cảnh phải khớp lưới cũ (tỉ lệ bất đẳng hướng, ellipse tầm đánh).

## 3. Hệ thống NPC / nhân vật

- Đối tượng `NPC : MonoBehaviour` (một lớp cho NPC, quái, người chơi khác — `NPCManager.npcDictionary/enemyDictionary/playerDictionary`):
  máy trạng thái `StateMachine` (Standing, Walk, Sprint, Jumping, Attack, Magic, Skill, Hurt, Death, Sit, BlurAttack, BlurMove, RunAttack,
  RunAttackMany, ManyAttack, RushAttack; `RunAttackCoordinator`), `JXAnimationController`, `JXMovement`, `JXController`, `JXEquipment`,
  `AbilityHolder`, `NpcFindPath`, `KNpcResList pNpcResList` (bảng trạng thái JX1), `_weaponResId/_aoResId/_nonResId/_horseResId/
  _phiphongResId/_typeHorse`, `m_nMenuState/m_bSleepState` (JX1), `NpcCellIndex` (bucket theo ô để truy vấn nhanh),
  `NpcCollisionLite`, `NpcSpawnerQueue` (sinh dần theo ngân sách khung), `NPCSelectionService`, `NpcStandSoundController`.
- Model: `NPCResDictSO.FindByPrefabName(byte[])` → `NPCResDict {npcResType, NPCSettingIdx, npcResPrefab}` — **khoá là tên res JX1**
  (`nNPCResType`, bảng `npcres`), không phải template id; `JXAnimationController.SetNPCResType`.
- Trang bị: `MeshSODict {ArmorSO/ArmorFemaleSO, WeaponSO, HelmetSO/HelmetFemaleSO, HorseSO, PhiphongSO/PhiphongFemaleSO, *TempSO}` →
  `ItemSO/MeshObject {nIdx, mName, prefab/AssetReference}` — **mesh riêng cho từng dòng res 2.0** (áo, nón, vũ khí, ngựa, phi phong,
  bội sức); `JXEquipment` giữ bộ xương cố định (`_persistentSkeleton`, `_bodyBoneMap`), ghép mesh bằng
  **`SkeletonTransplant.Transplant(srcInstance, targetBoneMap, meshHolder, fallbackBone)`** (khớp theo tên xương, `BaseName` bỏ hậu tố,
  `SanitizeBones`) — đúng cách ta làm ở `Scn3DNpc.set_costume` (3D-76); nón gắn `_headBone` với `_headRestLocalToBody`; vải
  `MagicaCloth2` (áo choàng/tóc) có `ColliderComponent` theo tên xương; `WeaponSkinResDictSO.GetSkinPrefab(gateway, series)` (skin vũ khí
  theo loại vũ khí × ngũ hành); ngựa: `MountFollowBehavior`, `JX.RopeChain.LeashHorseFollower/Manager` (ngựa **dắt theo** bằng dây Obi
  khi không cưỡi), `_mountHiddenRenderers` (ẩn phần thân khi lên ngựa).
- Ngân sách hiển thị: `NPCManager.maxRenderedPlayers`, `RENDER_CAP_*`, `PlayerRenderGovernor`, `PlayerSpawnLimiter`, `ItemSpawnLimiter`,
  `VFXSpawnLimiter`, `PerfTierResponder/HeatTierGovernor` (giảm chất lượng theo nhiệt/FPS), `FakePlayerSpawner/BotData` (người chơi giả
  để thử), `JX.Ambient.AmbientNpcRoute` (dân làng đi theo lộ trình knot + `AmbientSpeechPresenter` bong bóng thoại), `AnimalAutoMove`.
- So với ta: `KNpc` (luật 2.0) + `KNpc3DView` + `Scn3DNpc`; template → `cha_pic` bản tham khảo; khoá theo template id chứ không theo
  tên res; ta chưa có ngân sách sinh/hiển thị theo thiết bị, chưa có dân làng đi lại, ngựa dắt theo.

## 4. Hệ thống kỹ năng

- `Network.Resource.Header.KSkill` = `KSkill` JX1 nguyên vẹn (`m_szPreCastEffectFile`, `m_szManPreCastSoundFile/FMPreCastSoundFile`,
  `m_eLRSkillInfo`, `m_nCharActionId` (CLIENTACTION), `m_eMissleFollowKind`, `m_eMisslesGenerateStyle`, `m_bNeedShadow/m_nMaxShadowNum`,
  5 mảng `KMagicAttrib` Missle/Damage/Immediate/State/Plus, `m_nEquiptLimited/HorseLimited`, `m_nDoHurtP`, `m_nSeries`…) — đúng các cột
  `skills.txt` mà `KSkill.cpp` của zone ta đọc. `DataSkill/SkillList/PlayerSkillsData/SkillEquipment` = sổ kỹ năng + ô phím.
- **Đạn trên client** (`Missle : MonoBehaviour`, `MissleManager`): `KMissleDataStruct` = `KMissle` JX1 (mọi trường `m_nHeight/
  m_nHeightSpeed/m_nZAcceleration/m_nCollideRange/m_nDamageRange/m_nKnockBack/m_nStunTime/m_eMoveKind/m_eFollowKind/m_nRefPX…`),
  `SyncKMissleData` từ gói `OBJ_MISSLE_SYNC`, `FindMissleForServerCollision`, `OnMissleCollide`, `MissileCollisionJob` (Burst job),
  `MissleBurstMath`, `MissleSyncBoom`, `m_npcLastHitFrame`/`MIN_FRAMES_BETWEEN_HITS`, `PHYSICS_FRAME_TIME`, `MISSILE_LIFETIME_SCALE`,
  `s_relationCache` — client mô phỏng đạn và va chạm y hệt `KMissle` của client 2.0 (bản của ta chuyển việc này sang zone: đạn là gói
  `G2C_MISSLE`, client chỉ vẽ).
- Hình đạn: `MissleView` + `MissleResDictSO` → `MissleResData {MissleResId, MisslePrefab, SpecialEffectPrefabs[] theo trạng thái,
  Variants (skinKey)}` — **khoá = `MissleResId`** (dòng `missles.txt`/`MissleRes` 2.0), `TMissleRes {AnimFileName, nTotalFrame,
  nInterval, nDir, SndFileName}` (chính bảng `missles.txt`), `KSkillSpecial` (hiệu ứng phụ `m_pMissleRes/m_pMissleView`, pool);
  thiếu prefab → vẽ **hình cầu** thay (`[MissleView][SPHERE]`, `spherePrefab`); `VFXManager` + `VfxLod/VfxInstanceTuning/VFXSpawnLimiter`
  (giới hạn hiệu ứng theo nhóm), `PrecastVFXDictSO` (hiệu ứng tụ khí theo tên `PreCastEffectFile`), `StateResDictSO.GetVFX(stateID)` +
  `KStateSpr.PlayVFX` (trạng thái → VFX quanh người, `_buffRoot`), `HitElementSO` (hiệu ứng trúng theo ngũ hành), enum `EffectSkill`
  (Rồng, Bổng, Côn Lôn, Võ Đang, Thiên Nhẫn, Nga My, Thúy Yên), `EffectHit` (Ngạo Tuyết Tiêu Phong, Thiên Vương, Băng Tung Vô Ảnh),
  `JX_LockVFX`, `JX_SkillFollowTarget/MoveFront/ReturnPool`, `BlurVFX/GhostFade/MeshTrail` (bóng mờ, vệt mesh), `CameraShake`,
  `DamageText*` (số bay, `DamageNumbersPro`).
- Luồng thi triển (`SkillService`): `RANGED_PARTICULAR_OFFSET` (100+ ám khí như `EqtLimit` ta), `SKILL_RANGE_CAST_OFFSET/
  MOVE_STOP_OFFSET/AUTOMOVE_STOP_OFFSET`, **`PendingCastSkillId/LostCastCount/LastCastAckTime/PendingCastEchoSeen`** (đợi server dội
  lệnh, đếm lệnh mất — chính lỗi "server drop im lặng" ta gặp ở 3D-74), `s_localSkillCdUntil` (hồi chiêu cục bộ),
  `ATTACKACTION_EFFECT_PERCENT` (điểm phát đòn trong animation như 60 % của 2.0), `PostJump*` (thi triển sau khinh công), `Deferred
  blur` (dịch chuyển), `MoveSuppressedUntil` (chặn di chuyển ngay sau cast). Chuỗi log cho thấy AI đuổi đánh `FollowAttack` (=
  `FollowPeople`), `AddToOverLook`, `m_nTargetIdx`, `AutoAttack` vòng lặp, `DEFER OnSkillSelected` khi đang đánh dở, watchdog gỡ kẹt
  trạng thái đánh.
- So với ta: `skill_map.json` ghép theo id kỹ năng → prefab bản tham khảo (234/343); họ ghép theo `MissleResId` nên mọi kỹ năng dùng
  chung một dòng hình tự khớp và biến thể theo skin; zone ta bắn đạn nên client khỏi mô phỏng (đỡ lệch) nhưng số hiệu ứng phải tự ghép.

## 5. Hình chuyển động (animation) và model

- **Mecanim**: `ACDictSO {MalePlayerAnimator, FemalePlayerAnimator}` (2 AnimatorController người chơi), `PlayerAnimation` tham số
  `STATE` (int), `GATEWAY` (int = loại vũ khí/"cửa" — `WeaponSkinResDictSO.TryGetEntry(gateway)`, `PlayerAttackAnimData.GATEWAY_COUNT`),
  `TYPE_HORSE`, `SWITCH_RIDE`, `CHECK_PK`, `SKILL_PARAM` (id động tác kỹ năng, `RestartAnim(skillParam)`), `STYLE_PARAM` (biến thể ngẫu
  nhiên: `HasStyleParamState/HasStyleParamSkillParam`), `SetRandomIdleParam`; `JXAnimationController` cho NPC (`SetGateWay/SetNPCResType/
  SetTypeHorse/SetIsRide`, `RIDE_MOUNT/RIDE_DISMOUNT`); `AnimatorRebinder` (đổi controller/mesh mà không mất trạng thái),
  `AnimVariantDictSO.GetVariant(sex, layer, animTaskType, gateway)` → `AnimVariantEntry {Swaps: BaseClip → NewClip}` với
  `AnimLayer {Base, Combat, Horse}`, `AnimTaskType {Idle, Walk, Run, Skill0, Skill1, SkillSpecial, Sit, Hurt, Death, Jump}` → dùng
  `AnimatorOverrideController` thay clip theo vũ khí/giới; `PlayerAttackAnimData.GetAttackLength(isFemale, gateway, AnimationType
  {Attack_0, Attack_1, Attack_Special, Magic, Skill, Hurt, Death, Idle, Walk, Run}, LayerContext)` bảng độ dài đòn (đồng bộ với
  `AttackFrame` server); tên state trong chuỗi: `Idle`, `IDLE0/IDLE1`, `Idle_Horse`, `NormalRun`, `RideRun`, `FreeAttack`,
  `EndRunAttack`, `FirstRun`, `KhinhCong`, `Villager_Idle`, lớp `IdleBreakLayer` (động tác nghỉ ngẫu nhiên), `SimpleAnimatorState`.
- Máy trạng thái (§3) có **đánh khi chạy** (`RunAttackState`, `RunAttackManyState`, `Ons2cSetRunAttackTag` từ server), **đánh xông**
  (`RushAttackState`), combo (`ManyAttackState`), **khinh công** (`KhinhCongState/Cycle/Phase`, `PlayerJumpHelper`, `JumpingState/
  LandingState`, `SprintState`), bóng mờ dịch chuyển (`BlurMoveState/BlurAttackState`, `GhostFade`), ngồi (`SitState`, `dirSitting`),
  chết (`DeathState`, `DeathTrace`), bị đánh (`HurtState`).
- Model: bộ xương cố định + mesh ghép (`SkeletonTransplant`), vải `MagicaCloth2`, dây `Obi`, `MeshSimplify` LOD nướng, `PreviewModelAtlasLoader`
  (khung xem trước nhân vật `previewmodelframes`), `PlayerVisualBudget`, `PlayerNormalMapToggle`, `GPUQuality/DeviceTier`.
- So sánh: 剑网江湖 dùng `Animation` legacy theo bone prefab (`xx01/zp01/…`, nhóm `anim_group` theo vũ khí, `AnimStator`, 30 fps đoạn
  kỹ năng) — ta port thẳng vào `Scn3DNpc` (glTF + `AnimationPlayer`, nhóm). VLTK3D dựng lại bằng Mecanim và tự làm animation (không
  đọc được clip vì bundle mã hoá); điểm giống: chọn bộ động tác theo **loại vũ khí** (gateway ↔ anim_group ↔ nhóm của ta) và theo ngựa;
  điểm hơn: lớp Combat/Horse tách, biến thể ngẫu nhiên, đánh khi chạy, khinh công.

## 6. Giao diện / tính năng (để đối chiếu nhóm E của lộ trình)
160 lớp `Ui*`: đăng nhập/chọn server/tạo nhân vật (`UiLogin`, `UiLoginServerView`, `UiCreateCharacter` với `CharacterSelectVoice`),
túi/trang bị/nhân vật/trạng thái (`UiInventory`, `UiCharacter`, `UiStatus`, `UiItemInfo`, `UiCalculator`), kỹ năng (`UiSkill`,
`UiSkillScreen`, `UiUpgradeSkill`, `SlotSkillPC`, `SkillDirectionJoystick`), chat (`UiChatScreen(PC)`, bong bóng, emoji, kênh),
tổ đội (`UITeamPanel`), bang hội (`UiGuild*`, `Tong*`), bạn bè, giao dịch (`UITrade`, `UIOTT`), cửa hàng/chợ (`UiNPCShop`, `UiCustomShop`,
`UIBuyProductStore`, `UiFleaMarket` đấu giá), nhiệm vụ (`UiFullQuest/UiMiniQuest`, `QuestService`), **auto-play** (`UiAutoPlay`,
`UiAP_Fight/Pick/Recovery/Other/Script`, `AutoPickMagicTable`, `APWaypoint`), hoạt động ngày, rương, quay may mắn, bảng xếp hạng
(`UiLadder`), chiến trường Tống Kim (`UiBattleFieldRank`), cài đặt đồ hoạ/điều khiển (`UiGraphicSetting`, `UiControlMode`, joystick
mobile), tải map nền (`UiDownloadContent`), phản hồi (`UiFeedback`), bàn phím (`PCKeyMap`, `PCActionBuffer`), tooltip, minimap/bản đồ lớn.

## 7. Số mặc định rút từ hàm khởi tạo (`vltk3d/defaults39.py Lop`) — để "làm theo" có con số

| Hệ | Lớp / trường | Giá trị mặc định | Ý nghĩa |
|---|---|---|---|
| Cảnh | `MapRegionController._regionSquare / _maxMapSize / _activeRegionLevel` | 20 m / 900 / 2 | ô region 20 m, vòng 2 region quanh người chơi (5×5 = 100 m) đang bật |
| | `_frustumPadding / _zoneEnterRadius / _zoneExitRadius / _intraRegionReevalDistance` | 5 m / 10 m / 14 m / 3 m | cắt theo frustum + zone con bật ở 10 m, tắt ở 14 m (trễ), tính lại sau mỗi 3 m đi |
| | `_zoneActivateFrames / _zoneDeactivateFrames / _regionCheckInterval` | 2 / 2 / 2 s | bật/tắt dần từng khung, kiểm region mỗi 2 s |
| Cỏ/cây | `InstancedMapGrassRenderer._activeRegionLevel / _checkInterval / _treeReevalDistance / _boundsPadding` | 2 / 0,5 s / 4 m / 200 m | instancing theo region, LOD theo chiều cao màn hình (`_protoLodScreenH`), tỉ lệ cỏ 3 mức `_mainPct 0,25 / 0,5 / 1` |
| | `GrassBudgetGovernor` | rớt < 45 fps giữ 5 s → giảm mức; lên > 55 fps giữ 5 s → tăng; làm mượt 15 khung; trần 1 500 | tự hạ số cỏ theo FPS |
| | `PlayerRenderGovernor._maxPlayers` | 25 (cùng ngưỡng 45/55 fps) | số người chơi vẽ đầy đủ |
| Sinh | `PlayerSpawnLimiter` PC / Android / iOS | 100 / 40 / 25 người; spawn 150 / 60 / 40; mesh gần 50 / 30 / 30 m; RAM 85 / 80 / 70 % | ngoài `nearMeshDist` chỉ giữ dữ liệu, không dựng mesh |
| | `SpawnFrameBudget` | 3 ms/khung (tải 24, bùng 12); PC 8/48/24 | ngân sách thời gian sinh thực thể mỗi khung |
| | `NPCManager.renderCapInterval` | 0,2 s | xét lại giới hạn vẽ 5 lần/giây |
| | `VFXSpawnLimiter` (bộ lite/normal) | cặp (số tối đa, giây) theo nhóm: 30/24, 4/40, 8/24, 50/35, 6/50, 12/30; RAM 80 % | trần hiệu ứng theo nhóm |
| | `FrameRateGovernor.IdleFps` | 30 | FPS khi rảnh/nền |
| Tên/máu | `WorldHeadLabelRenderer` | cỡ chữ không đổi theo khoảng cách (chuẩn 12 m, FOV 60), viền 0,12, thanh máu 1,2 × 0,12, tối đa 20 thanh; **một mesh gộp** cho mọi tên | vẽ tên bằng một draw call |
| | `PlayerHeadManager` | tối đa 30 nhãn (15 người, 12 quái, 10 NPC, 10 vật), sắp lại mỗi 5 khung, 30 m; thanh máu người chơi chọn lọc: 10 tổng / 5 trong 12 m, nhớ giao chiến 5 s (theo `fight mode`); quái 10 | ngân sách nhãn/máu |
| Số bay | `UiDamagePopup` | gộp sát thương trong cửa sổ 1 s (làm mới 0,06 s), tối đa 2 dòng chữ/giây, lặp ≥ 0,15 s; `showDamageInterval 0,2 s`, exp 3 s | chống ngập số |
| Đánh | `SkillService.EffectiveLostThreshold` | clamp(RTT × 5, 0,3 s, 2,5 s) | lệnh không được server dội trong ngưỡng = mất; `LostCastCount` leo 1..8 mức xử lý |
| | `GetCastCooldownFrames` | hồi chiêu = khung + 60 % khung đòn (`AddEffectFrameOffsetToCooldown`) | điểm phát đòn 60 % như 2.0 |
| | `MoveSuppress` | chặn gói di chuyển ngay sau cast (`ShouldSuppressMovePacket`) | tránh cắt đòn |
| Di chuyển | `SprintState.runSpeed / smoothTime / stopSqrDistance` | 5 m/s / 0,02 / 0,01 | blend tree `LocomotionBT / CombatLocomotionBT / HorseLocomotionBT` |
| | `JXMovement` | canh kẹt: `STALL_THRESHOLD`, `WP_RESEND` (gửi lại điểm đường), `COMPUTE_PATH_TIMEOUT`, `JUMP_DOING_ORPHAN`, `LOCK_LEAK` | tự gỡ kẹt di chuyển |
| | `InputReader` | 2 chế độ: `MouseClick` / **WASD**; phím kỹ năng cơ bản/kết hợp | |
| Nhấp | `NpcClickInteractor` | tia 1 000 m, thoại trong 3 m, kéo ≥ 8 px không tính nhấp, giữ 0,25 s; mốc chọn cao 1,96 m (trên ngựa) / 1,2 m | |
| | `ClickPCEffect` | dấu nhấp 0,5 s; dấu đích tới khi cách 0,5 m, hết hạn 30 s | |
| Camera | `MobileCameraController.minZoom / maxZoom` | 1 / 10 (Cinemachine OrbitalFollow) | `CameraShake` 0,2 s / 0,25 |
| Minimap | `MiniMapController` | quét 3 500 Mps, 32 Mps/điểm, chấm người chơi xoay theo camera + hình quạt hướng nhìn, điểm đường, đồng đội | |
| Mờ/bóng | `MeshTrail.trailInterval 0,5 s`, `ErodeController` (tan rã khi chết: 0,03/0,01 s, trễ 1,25 s) | | |

## 8. Điều nên lấy về cho JX NEXT — từng mục (ưu tiên theo giá trị / công)

**A. Làm ngay được (client Godot, không đổi ADR-008)**
1. **Bảo vệ luồng thi triển** (§7 Đánh): đếm lệnh mất theo `clamp(RTT×5, 0,3, 2,5) s`, hồi chiêu cục bộ, hoãn skill khi đang đánh dở, chặn gói đi ngay sau cast, watchdog trạng thái — sửa tận gốc "nhấp không thấy gì".
2. **Số bay gộp** (`UiDamagePopup`): cửa sổ 1 s, ≤ 2 dòng chữ/giây — ta đang bay từng đòn.
3. **Ngân sách nhãn/máu** (`PlayerHeadManager`, `WorldHeadLabelRenderer`): tối đa 30 nhãn, thanh máu chọn lọc theo giao chiến 5 s, cỡ chữ không đổi theo khoảng cách — ta vẽ mọi tên/máu (`_draw_names`).
4. **Dấu nhấp / dấu đích / ngưỡng kéo 8 px / giữ 0,25 s** và **WASD** (tuỳ chọn) — chuột 2.0 giữ, thêm chế độ.
5. **Minimap có hướng nhìn + điểm đường + đồng đội** (ta mới có chấm).
6. **Vòng tầm kỹ năng trên đất** (`RangeIndicatorYawPin`, texture `IndicatorTex_SpellGround_*`) cho kỹ năng chuột phải.

**B. Hiệu năng cảnh 3D (đúng phần "map đẹp hơn" nhưng vẫn mượt)**
7. **Stream cảnh theo region 20 m, vòng 2, zone con bật 10 m/tắt 14 m** — ta đang bật cả map + `visibility_range` từng node; chia `KScenePlace3D` theo ô và bật/tắt dần từng khung.
8. **Cỏ/cây instancing theo region** (`MultiMeshInstance3D` theo ô, LOD theo chiều cao màn hình, 3 mức tỉ lệ, tự hạ theo FPS 45/55) — ta chưa có cỏ sinh.
9. **Bộ điều tốc theo FPS** (`*Governor`: rớt < 45 fps 5 s → hạ, > 55 fps 5 s → nâng) cho cỏ, số người vẽ, hiệu ứng; **ngân sách sinh 3 ms/khung**; trần người chơi 100 (PC), mesh gần 50 m.
10. **Trần hiệu ứng theo nhóm** (`VFXSpawnLimiter`) — ta chưa giới hạn (343 hiệu ứng có thể chồng).

**C. Đúng luật JX1 (zone) — cần kiểm lại nhị phân Linux trước**
11. **Tầm đồng bộ thực thể theo 3×3 region** (`KRegion.ConnectRegion[]`, `ClientRegionIdx[]` 9 ô): JX1 gửi NPC trong 9 region quanh người chơi (±768/±1 536 Mps) — ta dùng khung nhìn 2.0 1 280×1 536 nên quái 3D chỉ hiện khi tới gần; đối chiếu `jx_linux_y` (`KSubWorld::SyncRegion`) rồi đổi `view_width/height` cho map 3D.
12. **Đánh khi chạy** (`Ons2cSetRunAttackTag`, `S2C_SETRUNATTACKTAG`) — gói JX1 zone ta chưa phát; **khinh công** (`KhinhCong*`) — kỹ năng JX2 VN; cân nhắc theo bảng `skills.txt` của bản Linux.
13. **Affine 3 điểm Mps ↔ cảnh** (`MapService.Calibrate`) để dùng thẳng bẫy/NPC/script JX1 trên cảnh 3D: thử với Ba Lăng (map 53 ↔ `world_baling`) trước; nếu lệch bố cục thì chỉ lấy vị trí NPC/bẫy theo mốc.

**D. Hình/animation**
14. **Từ điển đạn theo `MissleResId`** (+ biến thể skin) song song `skill_map.json`; trạng thái → VFX theo `StateSpecialId`; tụ khí theo `PreCastEffectFile`.
15. **Lớp animation** giới × Base/Combat/Horse × việc × loại vũ khí, idle ngẫu nhiên (`IdleBreak`), blend tree đi/chạy/cưỡi — `AnimationTree` của Godot làm được với clip bản tham khảo.
16. **Mesh trang bị mở rộng** (nón, phi phong, bội sức, ẩn thân khi lên ngựa) — hệ thống có sẵn từ 3D-76, thiếu model.
17. Bóng mờ/vệt (`MeshTrail` 0,5 s, `GhostFade`) ta đã có (3D-71); **tan rã khi chết** (`ErodeController`) chưa.

**E. Không lấy / không thể**
- Cảnh, model, animation, VFX của họ (mã hoá theo phiên, tài sản GrowX); mã C#; giao thức `KsgCodec`; client dày chạy đạn/AI trên client (ngược lựa chọn zone-authoritative của ta).

## 9. "Map của bản đó đẹp hơn — thay vào được không?"

Không thay được: 31 cảnh 3D của họ nằm trong bundle mã hoá theo phiên (28/31 còn không có trong bộ cài, tải từ CDN khi chơi) và là tài sản riêng. Cái làm cho map họ "đẹp" phần lớn là **cách vẽ** chứ không chỉ mô hình — và cách vẽ thì ta làm lại được trong Godot:
- URP + **Volume post-processing theo map** (`MapDictItem.MapVolume`, `PlatformVolumeProfile`): bloom, tone map ACES, chỉnh màu, vignette; **sương theo cao độ** (`Boxophobic.AtmosphericHeightFog`) và tán xạ mặt trời; SSR/SSAO có sẵn trong URP 17 (`ScreenSpaceReflectionPass`, `PhysicallyBasedSkyModel`, `VolumetricClouds*` — có trong assembly, chưa xác nhận bật).
- **Lightmap nướng** (`MTE.BakedLightmapKeeper`) + nắng thời gian thực + bóng (khoảng bóng 15 m ở máy yếu), ngày/đêm (`DayNightSettingController`), nước có phản chiếu (`URPWater`, `WaterReflection`), địa hình đổi sang mesh (`AmazingAssets.TerrainToMesh`), LOD nướng (`MeshSimplifyBakedLods`), cỏ/cây instancing dày (§7).
- Godot 4.7 có đủ tương đương: `WorldEnvironment` (glow đã có 3D-66, tonemap ACES, SSAO/SSIL, SSR, **volumetric fog / height fog**, sky vật lý), `DirectionalLight3D` bóng, `MultiMeshInstance3D`, lightmap bản tham khảo đã dùng. **Điều kiện**: đổi renderer từ GL Compatibility sang **Forward+/Mobile (Vulkan)** — điểm chờ chủ dự án từ 3D-66 (bloom vài pixel là do GL). Cảnh vẫn là 45 cảnh 剑网江湖 (đã đẹp về mô hình); nếu muốn bố cục làng JX1 như VLTK3D thì phải dựng cảnh mới (việc mỹ thuật), không phải việc mổ nhị phân.
