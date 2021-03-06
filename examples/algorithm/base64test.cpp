#include<hgl/algorithm/Crypt.h>
#include<hgl/io/MemoryOutputStream.h>
#include<iostream>

using namespace hgl;
using namespace hgl::io;
using namespace hgl::algorithm;

using namespace std;

int main(int,char **)
{
    constexpr uchar test_str[]="BASE64 Decode and Encode";          //测试编解码用字符串

    cout<<"Origin: "<<test_str<<endl;

    MemoryOutputStream mos;
    
    if(!base64_encode(&mos,test_str,strlen(test_str)))
    {
        cout<<"encode error!"<<endl;
        return(-1);
    }

    int len=mos.GetSize();

    uchar *tmp=new uchar[len+1];

    memcpy(tmp,mos.GetData(),len);
    tmp[len]=0;

    cout<<"Encode: "<<tmp<<endl;

    mos.ClearData();

    if(!base64_decode(&mos,tmp,len))
    {
        cout<<"decode error!"<<endl;
        return(-2);
    }

    delete[] tmp;

    len=mos.GetSize();
    tmp=new uchar[len+1];
    memcpy(tmp,mos.GetData(),len);
    tmp[len]=0;

    cout<<"Decode: "<<tmp<<endl<<endl;

    delete[] tmp;
    return 0;
}
