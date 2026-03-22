//#include <unordered_map>
//#include <vector>
//#include <string>
//#include <utility> 
//#include <initializer_list>
//
//template<typename Key, typename Value, typename Hash = std::hash<Key>>
//class OrderedMap {
//private:
//    std::unordered_map<Key, Value, Hash> data;
//    std::vector<Key> order;
//
//public:
//    OrderedMap() = default;
//
//    OrderedMap(std::initializer_list<std::pair<Key, Value>> init) {
//        for (const auto& [k, v] : init) {
//            insert(k, v);
//        }
//    }
//
//    // ????????????????????????????????????????????????
//    // Вставка / присваивание
//    // ????????????????????????????????????????????????
//    void insert(const Key& key, const Value& value) {
//        auto [it, inserted] = data.try_emplace(key, value);
//        if (inserted) {
//            order.push_back(key);
//        }
//        else {
//            // если ключ уже был — обновляем значение
//            it->second = value;
//        }
//    }
//
//    void insert(Key&& key, Value&& value) {
//        auto [it, inserted] = data.try_emplace(std::move(key), std::move(value));
//        if (inserted) {
//            order.push_back(it->first);  // ключ уже перемещён в map
//        }
//    }
//
//    Value& operator[](const Key& key) {
//        auto [it, inserted] = data.try_emplace(key);
//        if (inserted) {
//            order.push_back(key);
//        }
//        return it->second;
//    }
//
//    Value& operator[](Key&& key) {
//        auto [it, inserted] = data.try_emplace(std::move(key));
//        if (inserted) {
//            order.push_back(it->first);
//        }
//        return it->second;
//    }
//
//    // ????????????????????????????????????????????????
//    // Итерация в порядке вставки
//    // ????????????????????????????????????????????????
//    auto begin() const { return order.begin(); }
//    auto end()   const { return order.end(); }
//
//    auto cbegin() const { return order.cbegin(); }
//    auto cend()   const { return order.cend(); }
//
//    // Если хочешь итерировать по парам (key, value), как в map
//    class const_iterator {
//        typename std::vector<Key>::const_iterator it;
//        const OrderedMap* parent;
//
//    public:
//        using value_type = std::pair<const Key&, const Value&>;
//        using reference = value_type;
//        using pointer = const value_type*;
//        using difference_type = std::ptrdiff_t;
//        using iterator_category = std::forward_iterator_tag;
//
//        const_iterator(typename std::vector<Key>::const_iterator i, const OrderedMap* p)
//            : it(i), parent(p) {}
//
//        reference operator*() const {
//            const auto& k = *it;
//            return { k, parent->data.at(k) };
//        }
//
//        const_iterator& operator++() { ++it; return *this; }
//        const_iterator  operator++(int) { auto tmp = *this; ++it; return tmp; }
//
//        bool operator==(const const_iterator& other) const { return it == other.it; }
//        bool operator!=(const const_iterator& other) const { return !(*this == other); }
//    };
//
//    const_iterator cbegin_pairs() const { return { order.cbegin(), this }; }
//    const_iterator cend_pairs()   const { return { order.cend(),   this }; }
//
//    // ????????????????????????????????????????????????
//    // Полезные методы
//    // ????????????????????????????????????????????????
//    bool contains(const Key& key) const { return data.contains(key); }
//
//    size_t size() const { return order.size(); }
//
//    bool empty() const { return order.empty(); }
//
//    void clear() {
//        data.clear();
//        order.clear();
//    }
//
//    // Опционально: удаление (с сохранением порядка остальных)
//    void erase(const Key& key) {
//        if (data.erase(key)) {
//            auto it = std::find(order.begin(), order.end(), key);
//            if (it != order.end()) {
//                order.erase(it);
//            }
//        }
//    }
//};