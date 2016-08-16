#pragma once

namespace cocos2d
{
	namespace ui
	{
		class TextField;
	}
}

class LoginScene : public cocos2d::Layer
{
public:
    static cocos2d::Scene* createScene();

    virtual bool init();
    
    void menuCloseCallback(Ref* pSender);
	void menuConnectCallback(Ref* pSender);
    
    CREATE_FUNC(LoginScene);

private:
	ui::TextField* _nameField;
	ui::TextField* _idField;
	ui::TextField* _passField;

};