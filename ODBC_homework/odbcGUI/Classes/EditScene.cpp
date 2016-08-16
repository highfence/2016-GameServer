#include "pch.h"
#include "ui\UITextField.h"

#include "MyUtil.h"
#include "DbManager.h"

#include "EditScene.h"

Scene* EditScene::createScene()
{
	auto scene = Scene::create();

	auto layer = EditScene::create();

	scene->addChild(layer);

	return scene;
}

bool EditScene::init()
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
		CC_CALLBACK_1(EditScene::menuCloseCallback, this));

	closeItem->setPosition(Vec2(origin.x + visibleSize.width - closeItem->getContentSize().width / 2,
		origin.y + closeItem->getContentSize().height / 2));

	auto menu = Menu::create(closeItem, NULL);
	menu->setPosition(Vec2::ZERO);
	this->addChild(menu, 1);
#pragma endregion

	// 입력 컨트롤
	initTextField(nullptr);

	auto addLabel = Label::create("Add User", "fonts/HMFMPYUN.ttf", 40);
	addLabel->setColor(Color3B::WHITE);
	addLabel->setAnchorPoint(Vec2(0, 0));
	auto addItem = MenuItemLabel::create(addLabel, CC_CALLBACK_1(EditScene::menuAddCallback, this));
	
	auto checkLabel = Label::create("Check User", "fonts/HMFMPYUN.ttf", 40);
	checkLabel->setColor(Color3B::WHITE);
	checkLabel->setAnchorPoint(Vec2(0, 0));
	auto checkItem = MenuItemLabel::create(checkLabel, CC_CALLBACK_1(EditScene::menuCheckCallback, this));
	
	auto deleteLabel = Label::create("Delete User", "fonts/HMFMPYUN.ttf", 40);
	deleteLabel->setColor(Color3B::WHITE);
	deleteLabel->setAnchorPoint(Vec2(0, 0));
	auto deleteItem = MenuItemLabel::create(deleteLabel, CC_CALLBACK_1(EditScene::menuDeleteCallback, this));

	auto initLabel = Label::create("Init TextField", "fonts/HMFMPYUN.ttf", 40);
	initLabel->setColor(Color3B::WHITE);
	initLabel->setAnchorPoint(Vec2(0, 0));
	auto initItem = MenuItemLabel::create(initLabel, CC_CALLBACK_1(EditScene::initTextField, this));

	auto loginMenu = Menu::create(addItem, checkItem, deleteItem, initItem, nullptr);
	loginMenu->setPosition(500, 250);
	loginMenu->alignItemsVertically();
	addChild(loginMenu);

	return true;
}

void EditScene::menuAddCallback(Ref* pSender)
{
	auto id = _idField->getString();
	auto pw = _passField->getString();
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

	auto wId = MyUtil::utf8toWstring(id);
	auto wPw = MyUtil::utf8toWstring(pw);

	auto hStmt = DBManager::getInstance()->getHStmt();
	auto query = std::wstring();
	
	query = L"CALL addUser('" + wId + L"','" + wPw + L"');";
	if (SQLExecDirect(hStmt, (SQLWCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS)
		MessageBoxW(nullptr, L"유저 추가 실패.. 이미 존재하는 놈 같은데!!", L"오오류", MB_OK);
	else
		MessageBoxW(nullptr, L"유저 추가 성공!", L"서엉공", MB_OK);
}

void EditScene::menuCheckCallback(Ref* pSender)
{
	auto id = _idField->getString();
	auto pw = _passField->getString();
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

	auto wId = MyUtil::utf8toWstring(id);
	auto wPw = MyUtil::utf8toWstring(pw);

	auto hStmt = DBManager::getInstance()->getHStmt();
	auto query = std::wstring();

	//query = L"SELECT * FROM users WHERE UserID = '" + wId + L"' AND PassWord = '" + wPw + L"';";
	query = L"CALL checkUser('" + wId + L"','" + wPw + L"');";
	if (SQLExecDirect(hStmt, (SQLWCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS)
		MessageBoxW(nullptr, L"유저가 없다!", L"오오류", MB_OK);
	else
		MessageBoxW(nullptr, L"유저가 있다!", L"서엉공", MB_OK);

}

void EditScene::menuDeleteCallback(Ref* pSender)
{
	auto id = _idField->getString();
	auto pw = _passField->getString();
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

	auto wId = MyUtil::utf8toWstring(id);
	auto wPw = MyUtil::utf8toWstring(pw);

	auto hStmt = DBManager::getInstance()->getHStmt();
	auto query = std::wstring();

	query = L"CALL deleteUser('" + wId + L"','" + wPw + L"');";
	if (SQLExecDirect(hStmt, (SQLWCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS)
		MessageBoxW(nullptr, L"지우기 실패! 아이디나 패쓰워드를 확인해봐 `ㅅ`~", L"오오류", MB_OK);
	else
		MessageBoxW(nullptr, L"유저 삭제 성공!", L"서엉공", MB_OK);
}

void EditScene::menuCloseCallback(Ref* pSender)
{
	DBManager::getInstance()->disconnectDB();
	Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	exit(0);
#endif
}

void EditScene::initTextField(Ref* pSender)
{
	if (_idField != nullptr)
		this->removeChild(_idField);
	if (_passField != nullptr)
		this->removeChild(_idField);

	_idField = ui::TextField::create("user Id", "fonts/HMFMPYUN.ttf", 30);
	_idField->setCursorEnabled(true);
	_idField->setPosition(Vec2(150, 250));
	_idField->setAnchorPoint(Vec2(0, 0));
	addChild(_idField);

	_passField = ui::TextField::create("password", "fonts/HMFMPYUN.ttf", 30);
	_passField->setPasswordEnabled(true);
	_passField->setPasswordStyleText("*");
	_passField->setCursorEnabled(true);
	_passField->setPosition(Vec2(150, 150));
	_passField->setAnchorPoint(Vec2(0, 0));
	addChild(_passField);
}
