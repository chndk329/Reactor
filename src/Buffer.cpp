#include "Buffer.h"

Buffer::Buffer(u_int16_t sep) : sep_(sep)
{

}

Buffer::~Buffer()
{

}

void Buffer::append(const char *data, size_t size)         // 数据追加到buf_中
{
    buf_.append(data, size);
}

 void Buffer::appendwithsep(const char *data, size_t size) // 数据追加到buf_中，附带报文分隔符信息
 {
    if(sep_ == 0) 
    {
        buf_.append(data, size);
    }
    else if(sep_ == 1)
    {
        buf_.append((char *)&size, 4);          // 报文头部
        buf_.append(data, size);                // 报文信息
    }
    // else
    // {
            // 其他情况
    // }
 }

size_t Buffer::size()                                      // 返回buf_大小
{
    return buf_.size();
}

const char *Buffer::data()                                 // 返回buf_首地址
{
    return buf_.data();
}

void Buffer::clear()                                       // 清空buf_
{
    buf_.clear();
}

void Buffer::erase(size_t pos, size_t len)                 // 从buf_的pos位置开始，删除len个字节，pos从0开始
{
    buf_.erase(pos, len);
}

bool Buffer::pickmessage(std::string &ss)                  // 从buf_中拆分出一个报文，存放在ss中，如果buf_中没有报文，返回false
{
    if(buf_.size() == 0) return false;

    if(sep_ == 0) 
    {
        ss = buf_;      // 没有分隔符
        buf_.clear();
    }
    else if(sep_ == 1)
    {
        int len;
        memcpy(&len, buf_.data(), 4);

        if(buf_.size() < len + 4) return false;         // 报文不完整，退出

        // 从接收缓冲区取出一个报文
        ss = buf_.substr(4, len);

        buf_.erase(0, len + 4);                         // 从buf_删除已获取的报文
    }
    return true;
}
