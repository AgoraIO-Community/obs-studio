#include "qt-wrappers.hpp"
#include "window-init-agora-widget.hpp"
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "ag-http-request.hpp"
#include <json11.hpp>
#include <util/platform.h>

using namespace std;

AgoraInitWidget::AgoraInitWidget(QWidget *parent)
	: QDialog(parent),
	  main(qobject_cast<OBSBasic *>(parent)),
	  ui(new Ui::AgoraInitWidget)
{
	ui->setupUi(this);

	installEventFilter(CreateShortcutFilter());

    QStringList resolution_list;
    resolution_list<<"360P"<<"480P"<<"720P"<<"1080P";
    ui->resolution_box->addItems(resolution_list);

    char *path = os_get_agora_config_path_ptr("agora_obs_config.txt");
    ifstream file(path, ios::in);
    istreambuf_iterator<char> beg(file), end;
    string json(beg, end);
    update_ui_by_json(json);
    cout << json << endl;
}

AgoraInitWidget::~AgoraInitWidget()
{
}

void AgoraInitWidget::update_ui_by_json(string& json_str)
{
    std::string err;

    auto json = json11::Json::parse(json_str, err);

    if (json["url"].is_null()) {
    } else {
        string str_url = json["url"].string_value();
        ui->url_lineEdit->setText(str_url.c_str());
    }
    
    if (json["token"].is_null()) {
    } else {
        string token = json["token"].string_value();
        ui->token_lineEdit->setText(token.c_str());
    }

    if (json["appId"].is_null()) {
    } else {
        app_id = json["appId"].string_value();
        ui->textEdit_appid->setText(app_id.c_str());
    }
    
    if (json["channel"].is_null()) {
    } else {
        channel = json["channel"].string_value();
        ui->textEdit_channel->setText(QString(channel.c_str()));
    }

    if (json["resolution"].is_null()) {
    } else {
        resolution = json["resolution"].string_value();
        ui->resolution_box->setCurrentText(QString(resolution.c_str()));
    }

    if (json["uid"].is_null()) {
    } else {
        if (json["uid"].is_string()) {
            uid = json["uid"].string_value();
            ui->uid_value->setText(QString(uid.c_str()));
        } else {
            int tUid = json["uid"].int_value();
            string uid_str = to_string(tUid);
            uid = uid_str;
            ui->uid_value->setText(QString(uid_str.c_str()));
        }
    }

    if (json["stringified"].is_null()) {
    } else {
        if (json["stringified"].is_bool()) {
            stringified_uid = json["stringified"].bool_value();
            ui->stringified_checkbox->setChecked(stringified_uid);
        }
    }

    if (json["token"].is_null()) {
        token = "";
    } else {
        token = json["token"].string_value();
    }
}

void AgoraInitWidget::on_request_button_clicked()
{
    // 47.88.230.248:3000/channel/get
    if (ui->url_lineEdit->text().isEmpty()) {
        QMessageBox::warning(this, QString("Warning"), QString("Please input valid url for request"));
        return;
    }
    
    HttpGetPostMethod *http_request = new HttpGetPostMethod();
    
    string url = QT_TO_UTF8(ui->url_lineEdit->text());
    
    int url_length = url.length();
    int port_position = url.find(':');
    int sub_position = url.find('/');

    string host = url;
    string port = "";
    string subpath = "";
    
    if (port_position > 0) {
        int length = 0;
        
        if (sub_position > 0) {
            length = sub_position - port_position - 1;
        } else {
            length = url_length - port_position;
        }
        
        port = url.substr((port_position + 1), length);
        host = url.substr(0, port_position);
    }
    
    if (sub_position > 0) {
        subpath = url.substr(sub_position);
        
        if (port_position < 0) {
            host = url.substr(0, sub_position);
        }
    }
    
    int ret = http_request->HttpGet(host, port, subpath, "");
    
    if(ret == -1) {
        QMessageBox::warning(this, QString("Warning"), QString("Http Socket error!"));
        cout << "Http Socket error!" << endl;
    }
    if(http_request->get_return_status_code() != 200) {
        QMessageBox::warning(this, QString("Warning"), QString("request error"));
        cout << "Http get status code was: " << http_request->get_return_status_code() << endl;
    }
    string main_text = http_request->get_main_text();
    update_ui_by_json(main_text);
}

void AgoraInitWidget::on_saveButton_clicked()
{
	QString strUrl = ui->url_lineEdit->text();
	QString strUid = ui->uid_value->text();
	bool withStringifiedUid = ui->stringified_checkbox->isChecked();
	QString strToken = ui->token_lineEdit->text();
	QString strAppid = ui->textEdit_appid->text();
	QString strChannel = ui->textEdit_channel->text();
	QString strResolution = ui->resolution_box->currentText();

    std::string input_url = QT_TO_UTF8(strUrl.trimmed());
	std::string input_uid = QT_TO_UTF8(strUid.trimmed());
	std::string input_appid = QT_TO_UTF8(strAppid.trimmed());
    std::string input_token = QT_TO_UTF8(strToken.trimmed());
	std::string input_channel = QT_TO_UTF8(strChannel.trimmed());
    std::string input_resolution = QT_TO_UTF8(strResolution.trimmed());

	if (input_channel.empty() || input_channel.length() > 64)
    {
		QMessageBox::warning(this, QString("Warning"), QString("Please input valid channel for Agora"));
    }
    else if (input_appid.empty() || input_appid.length() != 32)
    {
		QMessageBox::warning(this, QString("Warning"), QString("Please input valid App ID"));
	}
	else if (!isAllDigit(input_uid) && !withStringifiedUid) {
		QMessageBox::warning(this, QString("Warning"), QString("Please apply stringified option"));
	}
    else
    {
		app_id = input_appid;
		channel = input_channel;
        resolution = input_resolution;
        stringified_uid = withStringifiedUid;

        if (input_token.length() > 0)
        {
            token = input_token;
        }
        else
        {
            token = "";
        }
        
        if (input_uid.length() > 0)
        {
            uid = input_uid;
        }
        else
        {
            uid = "";
        }

		QDialog::done(QDialog::Accepted);
	}

    char *path = os_get_agora_config_path_ptr("agora_obs_config.txt");
    cout << "agora config path " << path << endl;

    const string comma = ", ";
    const string mark = "\"";
    string save_appid = "\"appId\": " + mark + app_id + mark + comma;
    string save_url = "\"url\": " + mark + input_url + mark + comma;
    string save_channel = "\"channel\": " + mark + channel + mark + comma;
    string save_resolution = "\"resolution\": " + mark + input_resolution + mark + comma;
    string save_uid = "\"uid\": " + mark + input_uid + mark + comma;
    string save_stringified = "\"stringified\": " + string(withStringifiedUid ? "true" : "false");
    string save_token = "";

    if (input_token.length() > 0)
    {
        save_token = comma + "\"token\": " + mark + token + mark;
    }

    string save_string = "{" + save_appid + save_channel + save_url + save_resolution + save_uid + save_stringified + save_token + "}";
    ofstream outfile;
    outfile.open(path);
    outfile << save_string.data();
}
