#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <set>

template <typename Key, size_t N = 2>
class ADS_set {
public:
    class Iterator;
    using value_type = Key;
    using key_type = Key;
    using reference = key_type&;
    using const_reference = const key_type&;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = Iterator;
    using const_iterator = Iterator;
    using key_equal = std::equal_to<key_type>; // Hashing
    using hasher = std::hash<key_type>;        // Hashing
    
private:
    struct element{
        key_type key;
        bool isFree {true};
    };
    struct bucket{
        element* block{nullptr};
        size_type t{0}; //lockal depth
        ~bucket() {
            delete block;
        }
    };
    bucket** table{nullptr};
    size_type curr_size{0}, d{0};
    
    
    void indexexpansion();
    void split(size_type idx);
    size_type find_idx(const key_type&) const;
    element* check_bucket(size_type) const;
    element* find_pos(const key_type&) const;
    void insert_unchecked(const key_type& );
    bool try_insert(size_type idx, const key_type& key);

public:
    ADS_set();
    ADS_set(std::initializer_list<key_type> ilist): ADS_set{} { insert(ilist);}
    template<typename InputIt> ADS_set(InputIt first, InputIt last): ADS_set{} {insert(first, last);}
    ADS_set(const ADS_set &other ): ADS_set{} {
        for (const auto &key: other) insert_unchecked(key);
    }
    ~ADS_set() {
        for (size_type j {0}; j < (1 << d); ++j){
            if (table[j]){
                auto tmp = table[j];
                for (size_type i = j+(1 << (tmp->t)); i < (1<<d); i += (1 << (tmp->t))){
                    if (table[i]){ table[i] = nullptr; }
                }
                delete[] table[j]->block;
                delete table[j];
                table[j] = nullptr;
            }
            
        }
        delete[] table[(1 << d)]->block;
        
        delete []table;
    }
    
    ADS_set& operator=(const ADS_set &other ) {
        if (this == &other) return *this;
        ADS_set tmp {other};
        swap(tmp);
        return *this;
    }
    
    ADS_set& operator=(std::initializer_list<key_type> ilist) {
        ADS_set tmp{ilist};
        swap(tmp);
        return *this;
    }
    
    size_type size() const {return curr_size; }
    bool empty() const {return !curr_size;}
    
    size_type count(const key_type &key) const;
    iterator find(const key_type &key) const {
        element* pos {find_pos(key)};
        size_type idx {find_idx(key)};
        idx %= (1 << table[idx]->t);
        if (pos) return iterator{this, pos, idx};
        return end();
    }
    
    void clear() {
        ADS_set tmp;
        swap(tmp);
    }
    
    void swap(ADS_set &other) {
        /*instazv austauschen*/
        /* ALLE INSTANZVARIABLEN MUESSEN HIER SEIN, SOGAR DIE NEUE*/
        std::swap(table, other.table);
        std::swap(d, other.d);
        std::swap(curr_size, other.curr_size);
    }
    
    void insert(std::initializer_list<key_type> ilist);
    std::pair<iterator,bool> insert(const key_type &key) {
        element* pos {find_pos(key)};
        size_type idx;
        idx %= (1 << table[idx]->t);
        if (pos) {
            idx = find_idx(key);
            idx %= (1 << table[idx]->t);
            return {iterator{this, pos, idx}, false};
        }
        insert_unchecked(key);
        idx = find_idx(key);
        idx %= (1 << table[idx]->t);

        return {iterator{this, find_pos(key), idx}, true};
    }
    template<typename InputIt> void insert(InputIt first, InputIt last);
    
    size_type erase(const key_type &key) {
        element *pos {find_pos(key)};
        if (pos){
            pos->isFree = true;
            --curr_size;
            return 1;
        }
        return 0;
    }
    
    const_iterator begin() const {return const_iterator{this, &table[0]->block[0], 0};}
    const_iterator end() const {return const_iterator{this, &table[(1<<d)]->block[0], static_cast<size_type>(1<<d)};}
    
    void dump(std::ostream& o = std::cerr) const;
    
    friend bool operator==(const ADS_set &lhs , const ADS_set &rhs) {
        if (lhs.curr_size != rhs.curr_size) return false;
        for (const auto& key: rhs){
            if (!lhs.find_pos(key)) return false;
        }
        return true;
    }
    friend bool operator!=(const ADS_set &lhs , const ADS_set &rhs  ) { return !(lhs == rhs); }
};

template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {
    const ADS_set *container;
    element *pos;
    size_type idx;
   
    void skip(){
        while(idx != (1 << container->d) && pos->isFree){
            if (pos == &container->table[idx]->block[N-1]){
                pos = &container->table[++idx]->block[0];
                while(idx > idx % (1 << (container->table[idx]->t)) && idx != (1 << container->d)){
                      pos = &container->table[++idx]->block[0];
                    //std::cout << "skipping" << "\n";
                }
            }

            else
             ++pos;
            
        }
       // if (idx == (1<<container->d)) std::cout << "Hi" << "\n";
    }
public:
    using value_type = Key;
    using difference_type = std::ptrdiff_t;
    using reference = const value_type&;
    using pointer = const value_type*;
    using iterator_category = std::forward_iterator_tag;
    
    explicit Iterator(const ADS_set *container = nullptr, element *pos = nullptr, size_type idx = -1): container{container}, pos{pos}, idx{idx}{
        
        if (pos){
            skip();
        }
    }
    reference operator*() const { return pos->key; }
    pointer operator->() const { return &pos->key; }
    Iterator& operator++() {
        if (pos == &container->table[idx]->block[N-1]){
            pos = &container->table[++idx]->block[0];
               // std::cout << "Meow: " << idx % (1 << (container->table[idx]->t)) << "\n";
                while (idx > idx % (1 << (container->table[idx]->t)) && idx != (1<<container->d)){
                    pos = &container->table[++idx]->block[0];
                    //std::cout << "meowipping\n";
                }
            
        }
        else ++pos;  skip(); return *this;
    }
    Iterator operator++(int) { Iterator rc{*this}; ++*this; return rc; }
    friend bool operator==(const Iterator &lhs, const Iterator &rhs) { return lhs.pos == rhs.pos; }
    friend bool operator!=(const Iterator &lhs, const Iterator &rhs) { return lhs.pos != rhs.pos; }
};

template <typename Key, size_t N> void swap(ADS_set<Key,N>& lhs, ADS_set<Key,N>& rhs) { lhs.swap(rhs); }

template <typename Key, size_t N>
ADS_set<Key,N>::ADS_set(){
    table = new bucket*[2];
    table[0] = new bucket;
    table[1] = new bucket;
    table[0]->block = new element[N];
    table[1]->block = new element[N];
}

template <typename Key, size_t N>
void ADS_set<Key,N>::insert_unchecked(const key_type& key){
    size_type idx = find_idx(key);
    if(!try_insert(idx, key)){
        while(true){
            idx = find_idx(key);
            if (d == table[idx]->t) indexexpansion();
            else if(d > table[idx]->t){
                if (!try_insert(idx, key)){
                    split(idx);
                    if(try_insert(idx, key)) break;
                }
            }
        }
    }
}

template <typename Key, size_t N>
void ADS_set<Key,N>::indexexpansion(){
    bucket** new_table {new bucket*[(1<<(d+1))+1]};
    for (size_type i{0}; i < (1 << d); ++i){
        new_table[i] = new_table[i+(1<<d)] = table[i];
    }
    new_table[1<<(d+1)] = new bucket;
    new_table[1<<(d+1)]->block = new element[N];
    delete []table;
    table = new_table;
    ++d;
}

template <typename Key, size_t N>
void ADS_set<Key,N>::split(size_type idx){
    bucket* new_bucket {new bucket};
    new_bucket->block = new element[N];
   
    size_type bit {(idx>>table[idx]->t) & 1};
    size_type cnt {0};
    for (size_type i{0}; i < N; ++i){
        
        if (!table[idx]->block[i].isFree && (hasher{}(table[idx]->block[i].key)>>(table[idx]->t)&1) == bit){
            new_bucket->block[cnt++] = table[idx]->block[i];
            table[idx]->block[i].isFree = true;
        }
    }
    ++table[idx]->t;
    new_bucket->t = table[idx]->t;
    auto tmp = table[idx];
    for (size_type i {idx % (1 << (tmp->t-1))}; i < (1<<d); i += (1 << (tmp->t-1))){
        if (table[i] == tmp && ((i>>(table[i]->t-1))&1) == bit)
            table[i] = new_bucket;
    }
}

template <typename Key, size_t N>
typename ADS_set<Key,N>::size_type ADS_set<Key,N>::find_idx(const key_type& key) const{
    return hasher{}(key) & ((1 << d) - 1);
}

template <typename Key, size_t N>
typename ADS_set<Key,N>::element* ADS_set<Key,N>::check_bucket(size_type idx) const{
    for (size_type i{0}; i < N; ++i){
        if (table[idx]->block[i].isFree)
            return &table[idx]->block[i];
    }
    return nullptr;
}

template <typename Key, size_t N>

void ADS_set<Key,N>::dump(std::ostream& o) const{
    std::set<key_type>set_c;
    for (size_type i{0}; i < (1<<d); ++i){
        o << i << ": ";
        bool ans{false};
        size_type v {0};
        for (size_type k{0}; k < i; ++k){
            if (table[i] == table[k]){
                ans = true;
                v = k;
                break;
            }
        }
        if (ans) o << '[' << v << ']' << "\n";
        else{
        for (size_type j{0}; j < N; ++j){
            if (!table[i]->block[j].isFree){
                o << table[i]->block[j].key << " ";
                set_c.insert(table[i]->block[j].key);
            }
            else o << "-" << " ";
        }
        o << " t: " << table[i]->t  << "\n";
        }
    }
//    for (const auto& e: set_c){
//        std::cout << e << " ";
//    }
    std::cout << "\n";
}
//
//
template <typename Key, size_t N>
typename ADS_set<Key,N>::element* ADS_set<Key,N>::find_pos(const key_type& key) const{
    size_type idx {find_idx(key)};
    for (size_type i{0}; i < N; ++i)
        if(!table[idx]->block[i].isFree && key_equal{}(table[idx]->block[i].key, key)) return &table[idx]->block[i];
    return nullptr;
}

template <typename Key, size_t N>
typename ADS_set<Key,N>::size_type ADS_set<Key,N>::count(const key_type& key) const{
    return !!find_pos(key);
}

template <typename Key, size_t N>
void ADS_set<Key,N>::insert(std::initializer_list<key_type> ilist){
    for (const auto& key: ilist){
        if (!find_pos(key)) insert_unchecked(key);
    }
    
}
template <typename Key, size_t N>
template<typename InputIt> void ADS_set<Key,N>::insert(InputIt first, InputIt last){
    for (auto it = first; it != last; ++it){
        if (!find_pos(*it)) insert_unchecked(*it);
    }
}

template <typename Key, size_t N>
bool ADS_set<Key,N>::try_insert(size_type idx, const key_type& key){
    element* block_el = check_bucket(idx);
    if (block_el) {block_el->key = key; block_el->isFree = false; ++curr_size; return true;}
    return false;
}

#endif // ADS_SET_H
