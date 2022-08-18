#include <iostream>
#include <fstream>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "messages1.pb.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <string.h>
#include <unistd.h>
#include <sstream> 
#include <netinet/tcp.h>
#include <netdb.h>

#define adr "127.0.0.1"
#define port 19091

/*
теперь вместо использования curl надо сделать напрямую чтение из сокета
"http://127.0.0.1:19091/get?channel=test_rtmp_server"
надо соединиться с сервером 127.0.0.1:19091 и читать из него protobuf напрямую
поразбирайся что такое протокол http
алгоритм примерно такой:
после соединения записать в сокет запрос "GET /get?channel=test_rtmp_server"
затем из сокета вычивается служебная инфрмация, заголовки, куки и т.д - всё это
заканчивается символами \r\n\r\n и сразу после этого пойдут данные protobuf 
*/

using namespace google::protobuf::io;
using namespace std;

int readSize(char buffer[4]){
  int n = int((unsigned char)(buffer[0]) << 24 |
            (unsigned char)(buffer[1]) << 16 |
            (unsigned char)(buffer[2]) << 8 |
            (unsigned char)(buffer[3]));
return n;
}

int main() {
  char buf[4];int n;Msg msg;char *buf2;string channel;string video1;string audio1;
  int sock;struct sockaddr_in addr;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock < 0){
    perror("socket error");
    exit(1);
  }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(adr);

  if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
    perror("connect error");
    exit(2);
  }
  stringstream ss;
  ss << "GET /get?channel=test_rtmp_server HTTP/1.1\r\n"
  << "Host: 127.0.0.1\r\n"  
  << "Accept: */*\r\n" 
  << "Connection: close\r\n\r\n";
  string request = ss.str();

  if(send(sock, request.c_str(), request.length(), 0) != (int)request.length()) {
  perror("Error with sending socket");
  return 0;
  }
  char buffer[1];

  while(true){
    read(sock,buffer,1);
    if (buffer[0] == '\r'){
      read(sock,buffer,1);
      if (buffer[0] == '\n'){
        read(sock,buffer,1);
        if (buffer[0] == '\r'){
          read(sock,buffer,1);
          if (buffer[0] == '\n'){
            break;
          }
        }
      }
    }
  }
  //hz chto s etoi strokoi, vozmojno eshe 2 byte dlya \r\n

  while(true){
  read(sock,buf,4);
  n = readSize(buf);
  buf2 = new char[n];
  read(sock,buf2,n);
  msg.ParseFromArray(buf2,n);

  switch(msg.type()){
    //start
    case 1:{
    Start start = msg.start();
    channel = channel + "channel(" + start.channel() + " " + 
    to_string(start.start_ts()) + " " + to_string(start.tb_num()) +
    "/" + to_string(start.tb_den()) + ")";
    cout<<channel<<endl;
    break;
      }
      //stream
    case 3:{
    Stream stream = msg.stream();
    DataMediaType data = stream.datamt();
    CodecParameters codec = stream.avparams();

    //VideoMediaType videomt = 8;
    if (stream.type() == 0){
      VideoMediaType video = stream.videomt();
      video1 = video1 + "stream( V name = v_stream codec= " + stream.codec();
      switch(video.pix_fmt()){
        case 0:
        video1 = video1 + "yuv420p ";
        break;
        case 1:
        video1 = video1 + "uyvy422 ";
        break;
        case 2:
        video1 = video1 + "yuvj420p ";
        break;
        case 3:
        video1 = video1 + "nv12 ";
        break;
      }
      video1 = video1 + "bitrate= ";
      video1 = video1 + to_string(stream.bitrate()) + " ";
      video1 = video1 + "timebase= ";
      video1 = video1 + to_string(stream.tb_num()) + "/" + to_string(stream.tb_den()) + " ";
        video1 = video1 + "extradatasize= " + to_string((stream.extradata()).size()) + " ";
        video1 = video1 + to_string(video.width()) + "x" + to_string(video.height()) + " ";
        video1 = video1 + "fps= " + to_string(video.fps_num()) + "/" + to_string(video.fps_den()) + " ";
        video1 = video1 + "aspect= " + to_string(video.aspect_num()) + "/" + to_string(video.aspect_den()) + " ";
        cout<<video1<<" "<<channel<<endl;
        //AudioMediaType audiomt = 9;
    }else if (stream.type() == 1){
      AudioMediaType audio = stream.audiomt();
      audio1 = audio1 + "stream( " + "A name = a_stream " + "codec= " + stream.codec() + " ";
      audio1 = audio1 + "bitrate= " + to_string(stream.bitrate()) + " " + "timebase= " + to_string(stream.tb_num()) + "/" + to_string(stream.tb_den()) + " ";
        audio1 = audio1 + "extradatasize= " + to_string((stream.extradata()).size()) + " " + to_string(audio.freq()) + "Hz" + " ";
      audio1 = audio1 + "ssize=" + to_string(audio.ssize()) + " sample_fmt= ";
      switch(audio.sample_fmt()){
        case 0:
        audio1 = audio1 + "S16 ";
        break;
        case 1:
        audio1 = audio1 + "U8 ";
        break;
        case 2:
        audio1 = audio1 + "S32 ";
        break;
        case 3:
        audio1 = audio1 + "FLT ";
        break;
        case 4:
        audio1 = audio1 + "DBL ";
        break;
        case 5:
        audio1 = audio1 + "U8P ";
        break;
        case 6:
        audio1 = audio1 + "S16P ";
        break;
        case 7:
        audio1 = audio1 + "S32P ";
        break;
        case 8:
        audio1 = audio1 + "FLTP ";
        break;
        case 9:
        audio1 = audio1 + "DBLP ";
        break;
      }cout<<" "<<audio1<<"stereo "<<channel<<endl;
          // SubtitleMediaType subtitlemt = 12;
    }else if (stream.type() == 2){
      SubtitleMediaType subt = stream.subtitlemt();
      cout<<subt.aspect_num()<<" ";
        cout<<subt.aspect_den()<<" ";
        cout<<subt.header()<<" ";
        cout<<subt.width()<<" ";
        cout<<subt.height()<<" ";
        for (int i = 0; i < subt.metadata_size(); i++){
      SubtitleMediaType::Pair pair = subt.metadata(i);
      cout<<pair.key()<<" "<<pair.value()<<" ";
        }
        // DataMediaType datamt = 13;
    }else if (stream.type() == 3){
      for (int i = 0; i < data.info_size(); i++){
      DataMediaType::Pair pair = data.info(i);
      cout<<pair.key()<<" "<<pair.value()<<" ";
        }
    }
    break;
      }
      case 4:{
         MediaPacket packet = msg.mediapacket();
         Stream stream = msg.stream();
         DataMediaType data = stream.datamt();
         cout<<"packet(dts=";
         cout<<packet.dts();
         cout<<"["<<packet.dts()/1000<<"."<<packet.dts()%1000<<"] ";

         cout<<"pts=";
         cout<<packet.dts() + packet.pts_offset();
         cout<<"["<<(packet.dts() + packet.pts_offset())/1000<<"."<<(packet.dts() + packet.pts_offset())%1000<<"] ";

         switch(packet.frametype()){
          case 0:
          cout<<"I"<<" ";
          break;
          case 1:
          cout<<"P"<<" ";
          break;
          case 2:
          cout<<"B"<<" ";
          break;
         }
         cout<<"size="<<packet.data().size()<<" ";

         switch(packet.stream_id()){
          case 2:
          cout<<video1<<channel<<endl;
          break;
          case 3:
          cout<<audio1<<channel<<endl;
          break;
         }
         break;
      }
      case 5:{
           GopSlice gop = msg.gopslice();
           cout<<gop.ts_num()<<" ";
           cout<<gop.ts_den()<<" ";
           cout<<gop.duration_num()<<" ";
           cout<<gop.duration_den()<<" ";
          for (int i = 0; i < gop.packets_size(); i++){
       MediaPacket packet = gop.packets(i);
       cout<<packet.stream_id()<<" ";
         cout<<packet.dts()<<" ";
         cout<<packet.pts_offset()<<" ";
         cout<<packet.frametype()<<" ";
         cout<<packet.data()<<" ";
         cout<<packet.duration()<<endl;
        }
        break;
       }
  }
  delete[] buf2;
  }
}
