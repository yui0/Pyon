//---------------------------------------------------------
//	Pyon!
//
//		©2013 Yuichiro Nakada
//---------------------------------------------------------
// g++ -o pyon pyon.cpp -Iinclude -Llib.x86 -L/usr/X11R6/lib -lcatcake -lasound -lmad -lfreetype -lpng -ljpeg -lz -lGL -lXxf86vm -lpthread -lX11

#include <stdio.h>
#include "catcake_main.h"
#include "font.h"

// パラメータ
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		480
#define SCREEN_HZ		60
// マップチップ
#define MAPCHIP_WIDTH		16		// マップチップの幅（ピクセル）
#define MAPCHIP_HEIGHT		224		// マップチップの高さ（ピクセル）
#define MAPCHIP_WIDTH_COUNT	SCREEN_WIDTH/MAPCHIP_WIDTH+1	// マップチップの横の数
//#define MAPCHIP_HEIGHT_COUNT	SCREEN_HEIGHT/MAPCHIP_HEIGHT	// マップチップの縦の数
#define MAPCHIP_GROUND		-SCREEN_HEIGHT/2-MAPCHIP_HEIGHT/2
// プレイヤー
#define PLAYER_WIDTH		48	// 幅
#define PLAYER_HEIGHT		64	// 高さ
#define PLAYER_JUMP		20	// ジャンプの強さ
#define PLAYER_GRAVIRY		0.25	// 重力
// 敵
#define ENEMY_WIDTH		96/3	// 幅
#define ENEMY_HEIGHT		32	// 高さ
#define ENEMY_SPEED		-5	// 移動スピード
#define ENEMY_HIT_LENGTH	20	// 衝突判定の領域サイズ
#define ENEMY_MAX		20	// 最大出現数
// アイテム
#define ITEM_WIDTH		16	// 幅
#define ITEM_HEIGHT		16	// 高さ
#define ITEM_SPEED		-4	// アイテムのスピード
#define ITEM_MAX		20	// 最大出現数
#define COIN_POINT		100	// コインのポイント
#define COIN_FRAME		14	// コイン画像のフレームインデックス
#define DIAMOND_POINT		1000	// ダイアモンドのポイント
#define DIAMOND_FRAME		64	// ダイアモンドのフレームインデックス
// 背景
//#define BACKGROUND_WIDTH	600
#define BACKGROUND_WIDTH	1200
#define BACKGROUND_HEIGHT	600
#define SCROLL_SPEED		 0.0004
// 画像
#if defined(__ANDROID__)
#define RESOURCE_PATH		"./sdcard/berry/"
#else
#define RESOURCE_PATH		"./assets.pyon/"
#endif
#define IMAGE_PLAYER		"char.png"
//#define IMAGE_ENEMY		"enemy.png"
//#define IMAGE_ITEM		"icon0.png"
#define IMAGE_BACKGROUND	"bg.png"
#define IMAGE_MAP		"map2.png"
#define IMAGE_TITLE		"0627a.png"
// 音
#define TRACK_BGM1		0
#define TRACK_BGM2		1
#define TRACK_SE1		2
#define TRACK_SE2		3
#define BGM_VOL			240
#define BGM_MAIN		"bgm_maoudamashii_5_town26.mp3"	// http://maoudamashii.jokersounds.com
#define BGM_GAMESTART		"bgm_gamestart_1.wav"
//#define BGM_GAMEOVER		"bgm_gameover_1.mp3"
#define BGM_GAMEOVER		"se_kusano_08_girl_sonnaa.mp3"
#define SE_VOL			240
//#define SE_GAMESTART		"se_maoudamashii_system46.mp3"
#define SE_GAMESTART		"se_kusano_06_girl_ikimasu.mp3"	// http://www.otonomori.info/
#define SE_JUMP			"se_jump_short.mp3"	// ユウラボ8bitサウンド工房
//#define SE_PYUU			"se_pyuuuuu.mp3"	// ユウラボ8bitサウンド工房
#define SE_ITEM_GET		"itemget.wav"

#define MAX(a, b) ( ((a)>(b) ) ? (a) : (b) )
#define MIN(a, b) ( ((a)<(b) ) ? (a) : (b) )


bool intersect(ckVec *a, r32 aw, r32 ah, ckVec *b, r32 bw, r32 bh) {
	aw /= 2;
	ah /= 2;
	bw /= 2;
	bh /= 2;
	if (b->x-bw <= a->x+aw && b->y-bh <= a->y+ah && b->x+bw >= a->x-aw && b->y+bh >= a->y-ah) {
	//if (b->x <= a->x+aw && b->y <= a->y+ah && b->x+bw >= a->x && b->y+bh >= a->y) {
//		printf("(%f,%f)-(%f,%f) (%f,%f)-(%f,%f)\n", a->x, a->y, aw, ah, b->x, b->y, bw, bh);
		return true;
	} else {
		return false;
	}
}

int score, nextScore;
int game_frame;

class Game : public ckTask
{
public:
	Game();
	~Game();

private:
	virtual void onUpdate();
	void (Game::*Scene)();
	void SceneTitleInit();
	void SceneTitle();
	void SceneGameInit();
	void SceneGame();
	void SceneGameOver();
	void Init();

	FontTex font;			// フォント

	ckSprt title_sprt;

	ckScr *m_scr;
	ckSprt bg_sprt;			// 背景
	float bg_scroll;

	ckSprt map_sprt;		// マップ

	ckSprt player_sprt;
	int player_frame;		// アニメーション
	float player_vx, player_vy;	// 移動速度
	int player_jumpPow;		// ジャンプ力
	int player_jumpAble;		// ジャンプ可

	ckSprt enemy_sprt;
	int enemy_frame[ENEMY_MAX];	// アニメーション
	int enemy_time[ENEMY_MAX];	// 存在時間

	ckSprt item_sprt;
	int item_frame[ITEM_MAX];	// アニメーション
};

void newGame()
{
	ckNewTask(Game);
}

void Game::onUpdate()
{
	(this->*Scene)();
}
Game::Game() : ckTask(ORDER_ZERO)
{
	ckSndMgr::openSoundDevice(ckSndMgr::CHANNEL_NUM_STEREO, ckSndMgr::SAMPLE_RATE_44KHZ, 50);

	// 背景
	m_scr = ckDrawMgr::newScreen(ckID::genID());
	//m_scr->setClearMode(false, true);
	//m_scr->setPerspective(false);
	//m_scr->moveLast();
//	m_scr->moveFirst();
	m_scr->moveBefore(ckDrawMgr::DEFAULT_2D_SCREEN_ID);

	SceneTitleInit();
}
Game::~Game()
{
	ckDrawMgr::deleteScreen(m_scr->getID());
	ckSndMgr::closeSoundDevice();
}

void Game::Init()
{
	int i;

	// 背景
	bg_sprt.init(2, m_scr->getID());
	bg_sprt.setTextureID(ckID_(IMAGE_BACKGROUND));
	bg_sprt.setBlendMode(ckDraw::BLEND_HALF, true);

	bg_sprt.dataPos(1).set(0, 0);	// 真ん中・スプライトの中央の座標
	bg_sprt.setDataSize(1, SCREEN_WIDTH, SCREEN_HEIGHT);
	bg_sprt.setDataUV(1, 0.0f, 0.0f, (float)SCREEN_WIDTH/BACKGROUND_WIDTH, 1);
	bg_scroll = 0;

	bg_sprt.setDataSize(0, SCREEN_WIDTH, SCREEN_HEIGHT);
	bg_sprt.setDataUV(0, 0.0f, 0.0f, (float)SCREEN_WIDTH/BACKGROUND_WIDTH, 1);

	// マップの生成
	map_sprt.init(MAPCHIP_WIDTH_COUNT, m_scr->getID());
	map_sprt.setTextureID(ckID_(IMAGE_MAP));
	map_sprt.setBlendMode(ckDraw::BLEND_HALF, true);
	for (i=0; i<MAPCHIP_WIDTH_COUNT; i++) {
		map_sprt.dataPos(i).set(i*16-SCREEN_WIDTH/2, MAPCHIP_GROUND+96);
		map_sprt.setDataSize(i, MAPCHIP_WIDTH, MAPCHIP_HEIGHT);
		map_sprt.setDataUV(i, 0.0f, 0.0f, 1, 1);
	}

	// プレイヤー
	player_sprt.init(1, ckDrawMgr::DEFAULT_2D_SCREEN_ID);
	player_sprt.setTextureID(ckID_(IMAGE_PLAYER));
	player_sprt.setBlendMode(ckDraw::BLEND_HALF, true);

	player_sprt.dataPos(0).set(24-SCREEN_WIDTH/2, MAPCHIP_GROUND+96+32+MAPCHIP_HEIGHT/2);
	player_sprt.setDataSize(0, PLAYER_WIDTH, PLAYER_HEIGHT);
	player_sprt.setDataUV(0, 0.0f, 0.0f, 1.0/15, 1.0/8);
	player_frame = 0;
	player_jumpPow = -1; // ジャンプ力
	player_jumpAble = 1; // ジャンプ可

	// 敵
	/*enemy_sprt.init(ENEMY_MAX, ckDrawMgr::DEFAULT_2D_SCREEN_ID);
	enemy_sprt.setTextureID(ckID_(IMAGE_ENEMY));
	enemy_sprt.setBlendMode(ckDraw::BLEND_HALF, true);

	for (i=0; i<ENEMY_MAX; i++) {
		enemy_sprt.dataPos(i).set(-ENEMY_WIDTH-SCREEN_WIDTH/2, 0);
		enemy_sprt.setDataSize(i, ENEMY_WIDTH, ENEMY_HEIGHT);
		enemy_sprt.setDataUV(i, 0.0f, 0.0f, 1.0/3, 1.0);
		enemy_frame[i] = 0;
	}

	// アイテム
	item_sprt.init(ITEM_MAX, ckDrawMgr::DEFAULT_2D_SCREEN_ID);
	item_sprt.setTextureID(ckID_(IMAGE_ITEM));
	item_sprt.setBlendMode(ckDraw::BLEND_HALF, true);

	for (i=0; i<ITEM_MAX; i++) {
		item_sprt.dataPos(i).set(-ITEM_WIDTH-SCREEN_WIDTH/2, 0);
		item_sprt.setDataSize(i, ITEM_WIDTH, ITEM_HEIGHT);
		item_sprt.setDataUV(i, 0.0f, 0.0f, 1.0/16, 1.0/5);
		item_frame[i] = 0;
	}*/
}
void Game::SceneGameInit()
{
	Init();

	// 全体
	score = 0;
	nextScore = 0;
	game_frame = 0;
	Scene = &Game::SceneGame;
	ckSndMgr::play(TRACK_BGM1, ckID_(BGM_MAIN), BGM_VOL, true);
}

void Game::SceneGame()
{
	int i;
	game_frame++;

	// 終了
	if (ckKeyMgr::isPressed(ckKeyMgr::KEY_ESCAPE)) {
		ckEndCatcake();
	}

	static int floorLen = 0;	// 床の長さ
	static int holeMax = 8;		// 穴の最大長(1〜8)
	static int floorMax = 0;	// 床の最大長(8〜1)
	static int heightMax = 10;	// 最大高さ(5〜10)

	// スコア加算
	score++;

	// スクロール
	bg_scroll += SCROLL_SPEED;
	if (bg_scroll >= 1.0) {
		bg_scroll = 0;
		bg_sprt.dataPos(1).set(0, 0);
	}
	if (bg_scroll+(float)SCREEN_WIDTH/BACKGROUND_WIDTH >= 1.0) {
		float a = SCREEN_WIDTH * (bg_scroll-(1-(float)SCREEN_WIDTH/BACKGROUND_WIDTH)) * 1/((float)SCREEN_WIDTH/BACKGROUND_WIDTH);
		bg_sprt.dataPos(1).set(-a, 0);
		bg_sprt.dataPos(0).set(SCREEN_WIDTH-a, 0);
	} else {
		// 範囲をずらす
		bg_sprt.setDataUV(1, bg_scroll, 0.0f, bg_scroll+(float)SCREEN_WIDTH/BACKGROUND_WIDTH, 1);
	}

	// 難易度調整
	if (score/4 > nextScore) {
		nextScore += 300;

		int lv = MIN(nextScore / 300, 7);	// Level: 0〜7
		holeMax = 1 + lv;			// 穴の最大長(1〜8)
		floorMax = 15/*8*/ - lv*2;			// 床の最大長(8〜1)
		heightMax = 2 + lv;			// 最大高さ(5〜10)
	}

	// プレイヤー処理
	if (player_jumpAble) {
		if (ckKeyMgr::isPressed(ckKeyMgr::KEY_SPACE) || ckKeyMgr::isPressed(ckKeyMgr::KEY_LBUTTON)) {
			// ジャンプ
			player_jumpPow = PLAYER_JUMP;
			ckSndMgr::play(TRACK_SE1, ckID_(SE_JUMP), SE_VOL, false);
		}
		player_frame = (game_frame / 7) % 4;
		if (player_frame >= 3) player_frame = 1;
		player_frame += 21;
	} else {
		// ジャンプ中
		player_frame = (game_frame / 4) % 3;
		player_frame += 6;
	}

	// 衝突判定
	if (player_sprt.dataPos(0).y < map_sprt.dataPos(3).y + 20 + MAPCHIP_HEIGHT/2
		/*&& map_sprt.dataPos(3).x < 4+32-SCREEN_WIDTH/2*/) {
		player_frame = 8;
		ckSndMgr::play(TRACK_BGM1, ckID_(BGM_GAMEOVER), BGM_VOL, false);
		Scene = &Game::SceneGameOver;
	} else if (player_jumpPow >= 0) {
		// 上昇
		player_sprt.dataPos(0).y += player_jumpPow;
		player_jumpPow--;
	} else {
		// 下降
		player_sprt.dataPos(0).y += player_jumpPow;
		player_jumpPow--;
		player_jumpAble = 0;
		if (map_sprt.dataPos(3).y != MAPCHIP_GROUND && player_sprt.dataPos(0).y < map_sprt.dataPos(3).y + 32 + MAPCHIP_HEIGHT/2) {
			// 着地
			player_sprt.dataPos(0).y = map_sprt.dataPos(3).y + 32 + MAPCHIP_HEIGHT/2;
			player_jumpAble = 1;
			player_jumpPow = 0;
		}
	}
	float ax = 1.0/15;
	float ay = 1.0/8;
	float sx = (player_frame % 15) * ax;
	float sy = (player_frame / 15) * ay;
	player_sprt.setDataUV(0, sx, sy, sx+ax, sy+ay);

	// 地面の移動
	for (i=0; i<MAPCHIP_WIDTH_COUNT; i++) map_sprt.dataPos(i).x -= 2;
	if (map_sprt.dataPos(0).x == -16-SCREEN_WIDTH/2) {
		// 地面のシフト
		for (i=0; i<MAPCHIP_WIDTH_COUNT; i++) {
			map_sprt.dataPos(i).x += 16;
			if (i < MAPCHIP_WIDTH_COUNT-1) map_sprt.dataPos(i).y = map_sprt.dataPos(i+1).y;
		}
		// 前回と同じ高さの床(または穴)の追加
		if (floorLen > 0) {
			floorLen--;
			map_sprt.dataPos(MAPCHIP_WIDTH_COUNT - 1).y = map_sprt.dataPos(MAPCHIP_WIDTH_COUNT - 2).y;
		} else if (map_sprt.dataPos(MAPCHIP_WIDTH_COUNT - 2).y == MAPCHIP_GROUND) {
			// 床の追加
			floorLen = MAX(1 + ckMath::rand(0, floorMax), floorMax-3);
			map_sprt.dataPos(MAPCHIP_WIDTH_COUNT - 1).y = MAPCHIP_GROUND + 16 * (5 + ckMath::rand(0, heightMax));
		} else {
			// 穴の追加
			floorLen = 1 + ckMath::rand(0, holeMax);
			map_sprt.dataPos(MAPCHIP_WIDTH_COUNT - 1).y = MAPCHIP_GROUND;
		}
	}

#if 0
	// 敵を生成
	if (game_frame % /*30*/40 == 0) {
		for (i=0; i<ENEMY_MAX; i++) if (enemy_sprt.dataPos(i).x < -ENEMY_WIDTH-SCREEN_WIDTH/2) break;
		if (i<ENEMY_MAX) {
			enemy_frame[i] = enemy_time[i] = 0;
			enemy_sprt.dataPos(i).set(SCREEN_WIDTH/2+30, ckMath::rand(-SCREEN_HEIGHT/2, SCREEN_HEIGHT/2-ENEMY_HEIGHT));
		}
	}

	for (i=0; i<ENEMY_MAX; i++) {
		if (enemy_sprt.dataPos(i).x < -ENEMY_WIDTH-SCREEN_WIDTH/2) continue;

		// 移動
		enemy_sprt.dataPos(i).x += ENEMY_SPEED;
		//enemy_sprt.dataPos(i).y += ckMath::cos_s32(time[i]*5*3.14/180);
		enemy_sprt.dataPos(i).y += ckMath::cos_s32(enemy_time[i]*2 % 180);

		// フレームアニメーション
		if (enemy_time[i] % 5 == 0) {
			enemy_frame[i] += 1;
			enemy_frame[i] %= 3;
		}
		float ux = (enemy_frame[i]%3)*1.0/3;
		float uy = (enemy_frame[i]/3)*1.0/1;
		enemy_sprt.setDataUV(i, ux, uy, ux+1.0/3, uy+1.0/1);

		// プレイヤーとの衝突判定
		if (intersect(&enemy_sprt.dataPos(i),
			enemy_sprt.dataW(i)/3, enemy_sprt.dataH(i)/4,
			  &player_sprt.dataPos(0),
			      player_sprt.dataW(0), player_sprt.dataH(0))) {
			// SE 再生
			ckSndMgr::play(TRACK_SE2, ckID_(SE_PYUU), SE_VOL, false);
//			ckEndCatcake();
			ckSndMgr::play(TRACK_BGM1, ckID_(BGM_GAMEOVER), BGM_VOL, false);
			Scene = &Game::SceneGameOver;
		}
		/*if (this.within(player, ENEMY_HIT_LENGTH)) {
			gameOver("鳥と衝突してしまいました. 残念!!");
		}*/

		// タイムを進める
		enemy_time[i]++;
	}

	// アイテムを生成
	if (game_frame % /*50*/60 == 0) {
		for (i=0; i<ITEM_MAX; i++) if (item_sprt.dataPos(i).x < -ITEM_WIDTH-SCREEN_WIDTH/2) break;
		if (i<ITEM_MAX) {
			int r = ckMath::rand(0, 100);
			if (r < 10) {
				item_frame[i] = DIAMOND_FRAME;
			} else {
				item_frame[i] = COIN_FRAME;
			}
			float ux = (item_frame[i]%16)*1.0/16;
			float uy = (item_frame[i]/16)*1.0/5;
			item_sprt.setDataUV(i, ux, uy, ux+1.0/16, uy+1.0/5);
			item_sprt.dataPos(i).set(SCREEN_WIDTH/2+30, ckMath::rand(-SCREEN_HEIGHT/2, SCREEN_HEIGHT/2-ITEM_HEIGHT));
		}
	}

	for (i=0; i<ITEM_MAX; i++) {
		if (item_sprt.dataPos(i).x < -ITEM_WIDTH-SCREEN_WIDTH/2) continue;

		// 移動
		item_sprt.dataPos(i).x += ITEM_SPEED;

		// 衝突判定
		if (intersect(&item_sprt.dataPos(i),
			item_sprt.dataW(i), item_sprt.dataH(i),
			  &player_sprt.dataPos(0),
			      player_sprt.dataW(0), player_sprt.dataH(0))) {
			// 削除
			item_sprt.dataPos(i).x = -ITEM_WIDTH-SCREEN_WIDTH/2-1;
			if (item_frame[i] == COIN_FRAME) {
				// スコア加算
				score += COIN_POINT;
				font.DrawEString(player_sprt.dataPos(0).x-PLAYER_WIDTH/2, player_sprt.dataPos(0).y, (char*)"+100", 50);
			} else {
				// スコア加算
				score += DIAMOND_POINT;
				font.DrawEString(player_sprt.dataPos(0).x-PLAYER_WIDTH/2, player_sprt.dataPos(0).y, (char*)"+1000", 50);
			}
			// SE 再生
			ckSndMgr::play(TRACK_SE2, ckID_(SE_ITEM_GET), SE_VOL, false);
		}
	}
	font.effect();
#endif

	// スコア表示
	char msg[256];
	sprintf(msg, "SCORE: %d", score);
	font.clear();
	font.DrawString(-SCREEN_WIDTH/2+16, SCREEN_HEIGHT/2-16, msg); // 0,0
	/*ckDbgMgr::drawString(0, 0, ckCol::FULL, 1, "SCORE: "+s);*/
}

void Game::SceneGameOver()
{
	//ckDbgMgr::drawString(0, 0, ckCol::FULL, 1, "Game Over");
	font.clear();
	font.DrawStringCenter(16, (char*)"Game Over", 32, 32);
	char msg[256];
	sprintf(msg, "SCORE: %d", score);
	font.DrawString(-SCREEN_WIDTH/2+16, SCREEN_HEIGHT/2-16, msg); // 0,0

	if (ckKeyMgr::isPressed(ckKeyMgr::KEY_ENTER) || ckKeyMgr::isPressed(ckKeyMgr::KEY_LBUTTON)) {
		SceneTitleInit();
	}
	// 終了
	if (ckKeyMgr::isPressed(ckKeyMgr::KEY_ESCAPE)) {
		ckEndCatcake();
	}
}

void Game::SceneTitleInit()
{
	// 消す
	Init();
	player_sprt.dataPos(0).set(-PLAYER_WIDTH-SCREEN_WIDTH, 0);
	//player_sprt.init(1, ckDrawMgr::DEFAULT_2D_SCREEN_ID);
	//enemy_sprt.init(ENEMY_MAX, ckDrawMgr::DEFAULT_2D_SCREEN_ID);
	//item_sprt.init(ITEM_MAX, ckDrawMgr::DEFAULT_2D_SCREEN_ID);

	// 背景
	bg_sprt.init(2, m_scr->getID());
	bg_sprt.setTextureID(ckID_(IMAGE_BACKGROUND));
	bg_sprt.setBlendMode(ckDraw::BLEND_HALF, true);

	bg_sprt.dataPos(1).set(0, 0);	// 真ん中・スプライトの中央の座標
	bg_sprt.setDataSize(1, SCREEN_WIDTH, SCREEN_HEIGHT);
	bg_sprt.setDataUV(1, 0.0f, 0.0f, (float)SCREEN_WIDTH/BACKGROUND_WIDTH, 1);

	// タイトル
	title_sprt.init(1, ckDrawMgr::DEFAULT_2D_SCREEN_ID);
	title_sprt.setTextureID(ckID_(IMAGE_TITLE));
	title_sprt.setBlendMode(ckDraw::BLEND_HALF, true);
	//title_sprt.dataPos(0).set(SCREEN_WIDTH/2-196/2, -SCREEN_HEIGHT/2+278/2);
	title_sprt.dataPos(0).set(0, 0);
	title_sprt.setDataSize(0, 196, 278);
	title_sprt.setDataUV(0, 0.0f, 0.0f, 1, 1);

	font.clear();
	font.DrawStringCenter(64, (char*)"Pyon!");
	font.DrawPStringCenter(84, (char*)"♥ピョン♥");
	font.DrawPStringCenter(-SCREEN_HEIGHT/2+80, (char*)"Press Return to Start");
	font.DrawPStringCenter(-SCREEN_HEIGHT/2+40, (char*)"(C)2013 YUICHIRO NAKADA");

	Scene = &Game::SceneTitle;
}

void Game::SceneTitle()
{
	if (ckKeyMgr::isPressed(ckKeyMgr::KEY_ENTER) || ckKeyMgr::isPressed(ckKeyMgr::KEY_LBUTTON)) {
		title_sprt.dataPos(0).set(-196-SCREEN_WIDTH, 0);
		ckSndMgr::play(TRACK_SE2, ckID_(SE_GAMESTART), SE_VOL, false);
		SceneGameInit();
		//Scene = &Game::SceneGame;
	}
	// フルスクリーン
	if (ckKeyMgr::isPressed(ckKeyMgr::KEY_F)) {
		ckSysMgr::toggleFullScreen(SCREEN_WIDTH, SCREEN_HEIGHT);
	}
	// 終了
	if (ckKeyMgr::isPressed(ckKeyMgr::KEY_ESCAPE)) {
		ckEndCatcake();
	}
}

ckMain()
{
	ckCreateCatcake("Pyon!", SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_HZ);

	ckResMgr::loadResource(RESOURCE_PATH IMAGE_FONT, true);
	ckResMgr::loadResource(RESOURCE_PATH IMAGE_FONT_P, true);

	ckResMgr::loadResource(RESOURCE_PATH BGM_MAIN, true);
	ckResMgr::loadResource(RESOURCE_PATH SE_GAMESTART, true);
	ckResMgr::loadResource(RESOURCE_PATH BGM_GAMEOVER, true);
	ckResMgr::loadResource(RESOURCE_PATH SE_JUMP, true);
	//ckResMgr::loadResource(RESOURCE_PATH SE_PYUU, true);
	ckResMgr::loadResource(RESOURCE_PATH SE_ITEM_GET, true);

	ckResMgr::loadResource(RESOURCE_PATH IMAGE_PLAYER, true);
	//ckResMgr::loadResource(RESOURCE_PATH IMAGE_ENEMY, true);
	//ckResMgr::loadResource(RESOURCE_PATH IMAGE_ITEM, true);
	ckResMgr::loadResource(RESOURCE_PATH IMAGE_BACKGROUND, true);
	ckResMgr::loadResource(RESOURCE_PATH IMAGE_MAP, true);
	ckResMgr::loadResource(RESOURCE_PATH IMAGE_TITLE, true);

	ckMath::srand(static_cast<u32>(ckSysMgr::getUsecTime()));

	newGame();

	ckStartCatcake();
#if ! defined(__ANDROID__)
	ckDestroyCatcake();
#endif
}
