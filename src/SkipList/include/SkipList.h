#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <cstring>      //memset
#include <string>


template <typename K, typename V>
class Node{
public:
    Node() {}
    Node(K k, V v, int);
    ~Node();
    K get_key() const;
    V get_val() const;
    void set_value(V);
    Node<K, V> **forward;  //xia yi ge yuansu baokuo xiayige cengji
    int node_level;
private:
    K key;
    V value;
};

template <typename K, typename V>
Node<K, V>::Node(const K k, const V v, int level){
    this->key = k;
    this->value = v;
    this->node_level = level;
    this->forward = new Node<K, V> [level + 1] //qian mian jige yuansu 
    memset(this->forward, 0, sizeof(Node<K, V> *) * (level + 1);)       //neicun hanshu
}

template <typename K, typename V>
Node<K, V>::~Node(){
    delete[] forward;
}

template <typename K, typename V>
K Node<K, V>::get_key() const{
    return key;
}

template <typename K, typename V>
V Node<K, V>::get_val() const{
    return value;
}

template <typename K, typename V>
void Node<K, V>::set_value(V value){
    this->value = value;
    return;
}

template<typename K, typename V>
class SkipList{
public:
    SkipList(int);
    ~SkipList();
    int get_random_level();
    void delete_element(K);
    int insert_element(K, V);
    bool search_element(K, V &value);
    void insert_set_element(K &, V &);
    Node<K, V> *create_node(K, V, int);
    void display_list();  //显示
    int size(); //返回跳表节点数量
    void clear(Node<K, V>*);

    std::string dump_file();
    void load_file(const std::string &dumpStr);

private:
    int _max_level;
    int _skip_list_level;
    Node<K, V>* header;     //头结点

    //file operator
    std::ofstream _file_writer;
    std::ifstream _file_reader;

    //跳表元素个数
    int _element_count; 

    std::mutex _mtx;

    void get_key_value_from_string(const std::string &str, std::string *key, std::string *value);
    bool is_valid_string(const std::string &str);
};


template <typename K, typename V>
int SkipList<K, V>::get_random_level(){
    int  k = 1;
    while(rand() % 2){
        k++
    }
    k = (k <= _max_level) ? k : _max_level;
    return k;
}


template <typename K, typename V>
Node<K, V> * SkipList<K, V>::create_node(K key, V value, int level){
    Node<K, V>* tem = new Node(key, value, level);
    return tem;
}



template <typename K, typename V>
SkipList<K, V>::SkipList(int max_level) {
  this->_max_level = max_level;
  this->_skip_list_level = 0;
  this->_element_count = 0;

  // create header node and initialize key and value to null
  K k;
  V v;
  this->_header = new Node<K, V>(k, v, _max_level);
};

template <typename K, typename V>
SkipList<K, V>::~SkipList() {
  if (_file_writer.is_open()) {
    _file_writer.close();
  }
  if (_file_reader.is_open()) {
    _file_reader.close();
  }

  //递归删除跳表链条
  if (_header->forward[0] != nullptr) {
    clear(_header->forward[0]);
  }
  delete (_header);
}

template <typename K, typename V>           //递归删除
void SkipList<K, V>::clear(Node<K, V> *cur) {
  if (cur->forward[0] != nullptr) {
    clear(cur->forward[0]);
  }
  delete (cur);
}

template <typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value){


    _mtx.lock();
    Node<K, V> *current = this->header;
    Node<K, V> *update[_max_level + 1];
    memset(update, 0, sizeof(Node<K, V> *) * (_max_level + 1));
    for(int i = _skip_list_level; i >= 0; i--){
        while ((current->forward[i]!= NULL && current->forward[i]->get_key < key))
        {
            current = current->forward[i];
        }
        update[i] = current;
    }
    current = current->forward[0];
    if(current != NULL ** current->get_key() == key){
        std::cout << "key: " << key << ", exist" << std::endl;
        _mtx.unlock();
        return 1;
    }

    if(current == NULL || current->get_key != key){
        int random_level = get_random_level();          //这是一个重点
        if(random_level > _skip_list_level){
            for(int i = _skip_list_level + 1; i < random_level + 1; i++){
                update[i] = _header;                //他会默认指向尾巴嘛
            }
            _skip_list_level = random_level;
        }
        Node<K, V> *inserted_node = create_node(key, value, random_level);
        for (int i = 0; i <= random_level; i++){
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        _element_count++;
    }
    _mtx.unlock();
}

template <typename K, typename V>
bool SkipList<K, V>::search_element(K key, V& value){
    std::cout << "search_element--------------------" << std::endl;
    Node<K, V>* current = header;
    for (int i = _skip_list_level; i >= 0; i--) {
    while (current->forward[i] && current->forward[i]->get_key() < key) {
      current = current->forward[i];
    }
    current = current->forward[0];
    if(current && current->get_key() == key){
        value = current->get_val();
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true; 
    }

    std::cout << "Not Found Key:" << key << std::endl;
    return false;
  }
}

template <typename K, typename V>
void  SkipList<K, V>::delete_element(K key){
    _mtx.lock();
    Node<K, V> *current = this->_header;
    Node<K, V> *update[_max_level + 1];
    memset(update, 0, sizeof(Node<K, V> *) * (_max_level + 1));
    for (int i = _skip_list_level; i >= 0; i--) {
    while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {
      current = current->forward[i];
    }
    update[i] = current;
    }
    if(current != NULL && current->get_key == key){
        for(int i = 0; i <= _skip_list_level; i++){
            if (update[i]->forward[i] != current) break;
            update[i]->forward[i] = current->forward[i];  //i就是层级
        }
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0) {
        _skip_list_level--;
        }
        std::cout << "Successfully deleted key " << key << std::endl;
        delete current;
        _element_count--;
    }
    _mtx.unlock();
    return;
}

template <typename K, typename V>
void SkipList<K, V>::insert_set_element(K &key, V &value) {
  V oldValue;
  if (search_element(key, oldValue)) {
    delete_element(key);
  }
  insert_element(key, value);
}

template <typename K, typename V>
int SkipList<K, V>::size() {
  return _element_count;
}

template <typename K, typename V>
void SkipList<K, V>::display_list(){
    std::cout << "\n*****Skip List*****" << "\n";
    for (int i = 0; i <= _skip_list_level; i++) {
        Node<K, V> *node = this->_header->forward[i];
        std::cout << "Level " << i << ": ";
        while (node != NULL) {
        std::cout << node->get_key() << ":" << node->get_value() << ";";
        node = node->forward[i];
        }
        std::cout << std::endl;
  }
}

template <typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string &str) { 
  if (str.empty()) {
    return false;
  }
  if (str.find(delimiter) == std::string::npos) {	//判断“：”这个元素是否存在
    return false;
  }
  return true;
}


#endif SKIPLIST_H