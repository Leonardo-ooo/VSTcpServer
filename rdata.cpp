#include "rdata.h"
#include <stack>

Rdata::Rdata(){
    action = content = uid = account = password = uname = friends = chatroom = "";
}

Rdata::Rdata(const std::string action, const std::string content ):action(action), content(content)
{
 
}

bool Rdata::stoxml(int op, char *buffer)
{
    std::string s = "<data><action>" + action + "</action>";
    switch(op)
    {
        case LOGIN_CHECK:
            s += "<content>" + content + "</content>";
            s += "<uid>" + uid + "</uid>";
            break;

        case LOGIN_SEND:
            s += "<uname>" + uname + "</uname>";
            s += "<friends>" + friends + "</friends>";
            s += "<chatroom>" + chatroom + "</chatroom>";
            break;

        case ULOG_SEND:
            s += "<uid>" + uid + "</uid><content>" + content + "</content>";
            break;

        case RLOG_SEND:
            s += "<rid>" + uid + "</rid><content>" + content + "</content>";
            break;

        case ACT_CONTENT:
            s += "<content>" + content + "</content>";
            break;
        
        case MESSAGE_SEND:
            s += "<content>" + content + "</content>";
            s += "<uid>" + uid + "</uid>";
            s += "<uname>" + uname + "</uname>";
            break;

        default:
            std::cout << "stoxml error! not a correct xml\n";
            return false;
    }
    s += "</data>";
	memcpy(buffer, s.c_str(), s.size() + 1);
    len = (int)strlen(buffer);
    return true;
}

bool Rdata::xmlparse(std::string data)
{
    std::stack<std::string> Label;
    std::string cur = "";
    for(int i = 0; i < int(data.size()); ++ i){
        if (data[i] == '$' && i + 1 < (int)data.size()) {
            cur += data[++ i];
        }
        else if (data[i] == '>') {
            cur += data[i];
            if(cur[1] == '/') {
                Label.pop();
            }
            else {
                Label.push(cur);
            }
            cur = "";
        }
        else if (data[i] == '<' && i + 1 < int(data.size()) && data[i + 1] == '/') {
            if (!Label.empty()) {
                std::string LastLabel = Label.top();
                if (LastLabel == "<action>") {
                    action = cur;
                }
                else if (LastLabel == "<account>") {
                    account = cur;
                }
                else if (LastLabel == "<content>") {
                    content = cur;
                }
                else if(LastLabel == "<password>"){
                    password = cur;
                }
                else if(LastLabel == "<uid>"){
                    uid = cur;
                }
                else if(LastLabel == "<uname>"){
                    uname = cur;
                }
                else if(LastLabel == "<friends>"){
                    friends = cur;
                }
                else if(LastLabel == "<chatroom>"){
                    chatroom = cur;
                }
            }
            cur = "<";
        }
        else {
            cur += data[i];
        }
    }
    if (Label.empty()) {
        return true;
    }
    else {
        return false;
    }
}
