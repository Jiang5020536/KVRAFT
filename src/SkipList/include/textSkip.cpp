#include "SkipList.h"
#include <iostream>
#include <string>

int main() {
    // 创建一个 SkipList 实例，最大层数为6
    SkipList<int, std::string> skipList(6);

    // 插入一些元素
    skipList.insert_element(1, "one");
    skipList.insert_element(3, "three");
    skipList.insert_element(7, "seven");
    skipList.insert_element(8, "eight");

    // 显示当前跳表内容
    std::cout << "Original SkipList:" << std::endl;
    skipList.display_list();

    // 序列化跳表到字符串
    std::string dumpStr = skipList.dump_file();
    std::cout << "\nSerialized SkipList:" << std::endl;
    std::cout << dumpStr << std::endl;

    // 创建一个新的 SkipList 实例，用于加载序列化数据
    SkipList<int, std::string> newSkipList(6);

    // 从字符串反序列化数据
    newSkipList.load_file(dumpStr);

    // 显示反序列化后的跳表内容
    std::cout << "\nDeserialized SkipList:" << std::endl;
    newSkipList.display_list();

    return 0;
}