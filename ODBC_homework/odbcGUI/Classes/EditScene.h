#pragma once

namespace cocos2d
{
	namespace ui
	{
		class TextField;
	}
}

class EditScene : public cocos2d::Layer
{
public:
    static cocos2d::Scene* createScene();

    virtual bool init();
    
	void menuCloseCallback(Ref* pSender);
	void menuAddCallback(Ref* pSender);
	void menuCheckCallback(Ref* pSender);
	void menuDeleteCallback(Ref* pSender);
    
	void initTextField(Ref* pSender);

    CREATE_FUNC(EditScene);

private:
	ui::TextField* _idField = nullptr;
	ui::TextField* _passField = nullptr;

};