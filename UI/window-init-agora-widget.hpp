
#pragma once

#include <QDialog>
#include <string>
#include <sstream>
#include <memory>

#include "ui_AgoraWidget.h"
class OBSBasic;

class AgoraInitWidget : public QDialog {
	Q_OBJECT
private:
	OBSBasic* main;
	std::unique_ptr<Ui::AgoraInitWidget> ui;

public slots:
void on_saveButton_clicked();
void on_request_button_clicked();
public:

	AgoraInitWidget(QWidget *parent);
	virtual ~AgoraInitWidget();

	std::string app_id;
    std::string channel;
    std::string token;
    std::string resolution;
    std::string uid;
    bool stringified_uid;

    void update_ui_by_json(std::string& json_str);

private:
	bool isAllDigit(std::string& str)  
	{  
		std::stringstream sin(str);  
		double d;  
		char c;  
		if (!(sin >> d))  
			return false;  
		if (sin >> c)  
			return false;  
		return true;  
	}
};
