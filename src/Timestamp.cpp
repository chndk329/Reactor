#include "Timestamp.h"

Timestamp::Timestamp()
{
    secsinceepoch_ = time(0);           // 获取当前时间
}

Timestamp::Timestamp(int64_t secsinceepoch) : secsinceepoch_(secsinceepoch)
{

}

Timestamp::~Timestamp()
{

}

// 当前时间
Timestamp Timestamp::now()
{
    return Timestamp();
}

time_t Timestamp::toint() const
{
    return secsinceepoch_;
}

std::string Timestamp::tostring() const
{
    char buf[80] = {0};
    tm *tm_time = localtime(&secsinceepoch_);
    snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d", 
            tm_time -> tm_year + 1900,
            tm_time -> tm_mon + 1,
            tm_time -> tm_mday,
            tm_time -> tm_hour,
            tm_time -> tm_min,
            tm_time -> tm_sec
        );
    return buf;
}