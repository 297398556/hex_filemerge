#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <map>
#include <algorithm>


// HEX类型
// DATA: data record， EOF_RECORD: end of file record
// EXT_SEGMENT_ADDR: 扩展段地址
enum RecordType{
    DATA                = 0x00,
    EOF_RECORD          = 0X01,
    EXT_SEGMENT_ADDR    = 0X02,
    START_SEGMENT_ADDR  = 0X03,
    EXT_LINEAR_ADDR     = 0X04,
    START_LINEAR_ADDR   = 0X05
};




//  HEX 结构
struct HexRecord{
    unsigned char length;
    unsigned short address;
    unsigned char type;
    std::vector<unsigned char> data;
    std::string hexrow;
};


// 解析HEX行
bool parseHex(const std::string& line, HexRecord& record){

    if(line.empty() || line[0] != ':') {
        return false;
    };

    std::vector<unsigned char> buf;
    
    // convert hex pairs to bytes
    // 转换hex pairs 为字节
    for(size_t i = 1; i + 1 <= line.size(); i += 2){
        
        unsigned int byte;

        std::string hexByte = line.substr(i, 2);
        // 从line字符串中从位置i开始的两个字符转换为16进制数值
        // 将字符串表示的十六进制格式转换为数值，存到byte中
        std::istringstream iss(hexByte);
        if(!(iss >> std::hex >> byte)){
            return false;
        }
        buf.push_back(static_cast<unsigned char>(byte));
    
    };

    // 最少hex记录长度为5：length(1) + address(2) + type(1) + checksum(1)
    // 1 + 2 + 1 + 1 = 5
    if(buf.size() < 5) return false;
    

    // 进行校验，所有字节相加应等于0
    unsigned char sum = 0;
    for(size_t i = 0; i < buf.size(); ++i){
        sum += buf[i];
    };
    if(sum != 0) return false;    

    // 记录hex中的各个属性
    record.length = buf[0];
    record.address = static_cast<unsigned short>(buf[1] << 8 | buf[2]);
    record.type = buf[3];
    
    record.data.clear();
    for(size_t i = 4; i < 4+record.length && i < buf.size() - 1; ++i){
        record.data.push_back(buf[i]);
    };

    record.hexrow = line; // 直接记录原始行
    // 解析成功
    return true;
};

// 生成新的HEX行
std::string generateHexRecord(unsigned char type, unsigned short addr, const std::vector<unsigned char>& data){
    
    std::vector<unsigned char> buf;
    buf.push_back(static_cast<unsigned char>(data.size()));
    buf.push_back(static_cast<unsigned char>((addr >> 8) & 0xFF));
    buf.push_back(static_cast<unsigned char>(addr & 0xFF));
    buf.push_back(type);
    for(size_t i = 0; i< data.size(); ++i){
        buf.push_back(data[i]);
    }

    // 计算校验和
    unsigned char sum = 0;
    for(size_t i = 0; i < buf.size(); ++i){
        sum += buf[i];
    };
    // 计算校验和取反加1，符合HEX文件规范
    buf.push_back(static_cast<unsigned char>(-sum));

    // 格式化输出 按照十六进制格式
    // :LLAAAATTDDDDDD.....CC
    std::ostringstream oss;
    oss << ':';
    for(size_t i = 0; i < buf.size(); ++i){
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(buf[i]);
    };
    return oss.str();    
}

// 比较函数，用于按照地址的大小排序
bool compareHexRecord(const HexRecord& a, const HexRecord& b){
    return a.address < b.address;
};


// 主程序入口
// 用于合并HEX文件
int main(int argc, char* argv[]){

    if(argc < 3){
        std::cerr << "Usage: " << argv[0] << " <inputfile.hex> <outputfile.hex>" << std::endl;
        return 1;
    };

    std::ifstream fileIn(argv[1]);
    std::ofstream fileOut(argv[2]);
    if(!fileIn || !fileOut){
        std::cerr << "打开文件失败" << std::endl;
        return 1;
    };

    // 以高地址作为索引组织数据，将所有数据按照地址分组，便于后续处理
    typedef std::map<unsigned short, std::vector<HexRecord>> HexMap;
    HexMap originalHexSegements;
    std::vector<HexRecord> otherRecords;
    bool hasEOF = false;
    HexRecord startLinearAddrRec; // 保存线性起始地址记录
    bool hasStartLinearAddr = false; // 是否存在线性起始地址记录

    HexRecord currentELA;
    std::vector<HexRecord> currentDatagroup;
    bool inELA = false;
    unsigned short currentHighAddr = 0;

    std::string line;

    // 第一步处理
    // 读取hex文件并处理每一行
    while(std::getline(fileIn, line)){
        HexRecord rec;
        if(!parseHex(line, rec)){
            std::cerr << "解析错误的hex行,直接输出原行: " << line << std::endl;
            fileOut << line << std::endl;
            continue;
        }

        if(rec.type == EOF_RECORD){
            otherRecords.push_back(rec);
            hasEOF = true;
            continue;
        }
        else if(rec.type == START_LINEAR_ADDR){
            // 保存起始线性地址记录，稍后在EOF前输出
            startLinearAddrRec = rec;
            hasStartLinearAddr = true;
            continue;
        }
        else if(rec.type == EXT_LINEAR_ADDR){
            // 提取高位地址值，这里需要正确提取扩展线性地址值
            if(rec.data.size() >= 2){
                currentHighAddr = static_cast<unsigned short>(rec.data[0] << 8 | rec.data[1]);
                
                // 如果已经在处理ELA记录，保存之前的数据组
                if(inELA){
                    originalHexSegements[currentELA.address] = currentDatagroup;
                    currentDatagroup.clear();
                }

                currentELA = rec;
                currentELA.address = currentHighAddr; // 保存实际高位地址
                inELA = true;
            }
            else {
                std::cerr << "警告: ELA记录的数据不完整: " << line << std::endl;
                otherRecords.push_back(rec);
            }
        }
        else if(rec.type == DATA && inELA){
            currentDatagroup.push_back(rec);
        }
        else{
            otherRecords.push_back(rec);
        }
    };

    // 如果最后还有未保存的ELA组，将其保存起来
    if(inELA && !currentDatagroup.empty()){
        originalHexSegements[currentELA.address] = currentDatagroup;
    }

    // 输出调试信息，显示分组情况
    std::cerr << "读取完成原始段数: " << originalHexSegements.size() << std::endl;
    for(HexMap::iterator it = originalHexSegements.begin(); it != originalHexSegements.end(); ++it){
        std::cerr << "高地址: 0x" << std::hex << it->first << ", 数据记录数: " << std::dec << it->second.size() << std::endl;
    }

    // 合并处理
    HexMap mergedHexSegments;
    for(HexMap::iterator it = originalHexSegements.begin(); it != originalHexSegements.end(); ++it){
        unsigned short origHigh = it->first;
        unsigned short newHigh = origHigh;
        
        // 将地址从0x8000映射到0xA000
        if((origHigh & 0xF000) == 0x8000){
            newHigh = static_cast<unsigned short>(0xA000 | (origHigh & 0x0FFF));
            std::cerr << "转换地址: 0x" << std::hex << origHigh << " -> 0x" << newHigh << std::endl;
        }

        std::vector<HexRecord>& target = mergedHexSegments[newHigh];
        std::vector<HexRecord>& source = it->second;
        target.insert(target.end(), source.begin(), source.end());
    }

    // 输出合并后段的数量
    std::cerr << "合并完成段数: " << mergedHexSegments.size() << std::endl;

    // 对每个段中的数据记录按低地址进行排序
    for(HexMap::iterator it = mergedHexSegments.begin(); it != mergedHexSegments.end(); ++it){
        std::vector<HexRecord>& records = it->second;
        std::sort(records.begin(), records.end(), compareHexRecord);
    }

    // 先输出非EOF和非起始线性地址的其他记录
    for(std::vector<HexRecord>::iterator it = otherRecords.begin(); it != otherRecords.end(); ++it){
        if(it->type != EOF_RECORD && it->type != START_LINEAR_ADDR){
            fileOut << it->hexrow << std::endl;
        }
    }

    // 输出各个合并后的ELA组和数据记录
    for(HexMap::iterator it = mergedHexSegments.begin(); it != mergedHexSegments.end(); ++it){
        unsigned short highAddr = it->first;

        // 生成新的ELA记录
        std::vector<unsigned char> elaData;
        elaData.push_back(static_cast<unsigned char>((highAddr >> 8) & 0xFF));
        elaData.push_back(static_cast<unsigned char>(highAddr & 0xFF));
        fileOut << generateHexRecord(EXT_LINEAR_ADDR, 0, elaData) << std::endl;
        
        // 输出该段内所有的数据记录
        std::vector<HexRecord>& records = it->second;
        for(std::vector<HexRecord>::iterator dataIt = records.begin(); dataIt != records.end(); ++dataIt){
            fileOut << dataIt->hexrow << std::endl;
        }
    }

    // 输出线性起始地址记录 (05类型)，放在EOF前
    if(hasStartLinearAddr){
        fileOut << startLinearAddrRec.hexrow << std::endl;
    }

    // 最后输出EOF记录
    if(hasEOF){
        // 找到并输出已有的EOF记录
        for(std::vector<HexRecord>::iterator it = otherRecords.begin(); it != otherRecords.end(); ++it){
            if(it->type == EOF_RECORD){
                fileOut << it->hexrow << std::endl;
                break;
            }
        }
    }else{
        // 如果没有EOF记录，生成一个标准的
        fileOut << ":00000001FF" << std::endl;
    }

    return 0;
}
