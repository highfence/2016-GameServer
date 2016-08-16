#include "pch.h"
#include "ui\UITextField.h"

#include "MyUtil.h"
#include "DbManager.h"

#include "EditScene.h"

#include "LoginScene.h"

Scene* LoginScene::createScene()
{
	auto scene = Scene::create();

	auto layer = LoginScene::create();

	scene->addChild(layer);

	return scene;
}

bool LoginScene::init()
{
	if (!Layer::init())
	{
		return false;
	}
#pragma region not_important
	auto visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();

	auto closeItem = MenuItemImage::create(
		"CloseNormal.png",
		"CloseSelected.png",
		CC_CALLBACK_1(LoginScene::menuCloseCallback, this));

	closeItem->setPosition(Vec2(origin.x + visibleSize.width - closeItem->getContentSize().width / 2,
		origin.y + closeItem->getContentSize().height / 2));

	auto menu = Menu::create(closeItem, NULL);
	menu->setPosition(Vec2::ZERO);
	this->addChild(menu, 1);
#pragma endregion

	// 입력 컨트롤
	_nameField = ui::TextField::create("odbc name", "fonts/HMFMPYUN.ttf", 30);
	_nameField->setCursorEnabled(true);
	_nameField->setPosition(Vec2(200, 350));
	_nameField->setAnchorPoint(Vec2(0, 0));
	addChild(_nameField);

	_idField = ui::TextField::create("user id", "fonts/HMFMPYUN.TTF", 30);
	_idField->setCursorEnabled(true);
	_idField->setPosition(Vec2(200, 250));
	_idField->setAnchorPoint(Vec2(0, 0));
	addChild(_idField);

	_passField = ui::TextField::create("password", "fonts/HMFMPYUN.ttf", 30);
	_passField->setPasswordEnabled(true);
	_passField->setPasswordStyleText("*");
	_passField->setCursorEnabled(true);
	_passField->setPosition(Vec2(200, 150));
	_passField->setAnchorPoint(Vec2(0, 0));
	addChild(_passField);

	auto loginLabel = Label::create("Connect!", "fonts/HMFMPYUN.ttf", 50);
	loginLabel->setColor(Color3B::WHITE);
	loginLabel->setAnchorPoint(Vec2(0, 0));
	auto loginItem = MenuItemLabel::create(loginLabel, CC_CALLBACK_1(LoginScene::menuConnectCallback, this));
	auto loginMenu = Menu::create(loginItem, nullptr);
	loginMenu->setPosition(500, 250);
	addChild(loginMenu);

	return true;
}

void LoginScene::menuConnectCallback(Ref* pSender)
{
	auto name = _nameField->getString();
	auto id = _idField->getString();
	auto pw = _passField->getString();

	// 입력 체크
	if (name == "")
	{
		MessageBoxW(nullptr, L"ODBC 이름을 입력해!", L"오오류", MB_OK);
		return;
	}
	if (id == "")
	{
		MessageBoxW(nullptr, L"유저 ID를 입력해!", L"오오류", MB_OK);
		return;
	}
	if (pw == "")
	{
		MessageBoxW(nullptr, L"비밀번호를 입력해!", L"오오류", MB_OK);
		return;
	}

	

	auto wName = MyUtil::utf8toWstring(name);
	auto wId = MyUtil::utf8toWstring(id);
	auto wPw = MyUtil::utf8toWstring(pw);
	if (DBManager::getInstance()->connectDB(wName, wId, wPw) == false)
		MessageBoxW(nullptr, L"DB 접속 실패!", L"오오류", MB_OK);
	else
	{
		MessageBoxW(nullptr, L"DB 접속 성공! `ㅅ`", L"으하하", MB_OK);
		auto newScene = EditScene::createScene();
		Director::getInstance()->pushScene(newScene);
	}
}


void LoginScene::menuCloseCallback(Ref* pSender)
{
	DBManager::getInstance()->disconnectDB();
	Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	exit(0);
#endif
}