#pragma once
#include <iostream>
#include <string>

class ProgressBar
{
public:
    ProgressBar(int max, int width):max(max), width(width){}
    void flush(int val)
    {
        double percentage = double(val) / max;
        int p1000 = percentage * 1000;
        int length_nempty = int(round(percentage * width));
        if (p1000 == last_reported) return;
        last_reported = p1000;
        std::cout << "\r["
            << std::string(length_nempty, '=')
            << std::string(width - length_nempty, ' ')
            << "] ("<<std::to_string(val)<<"/"<<std::to_string(max)<<" "<<std::to_string(p1000 / 10.0)<<"%)   ";
        std::cout.flush();
    }

private:
    int max, width;
    int last_reported = 0;
};
