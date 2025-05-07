// ht.h
#ifndef HT_H
#define HT_H

#include <vector>
#include <stdexcept>
#include <cmath>

typedef size_t HASH_INDEX_T;

// ---- Probers ----
template <typename KeyType>
struct Prober {
    HASH_INDEX_T start_, m_;
    size_t numProbes_;
    static const HASH_INDEX_T npos = (HASH_INDEX_T)-1;
    void init(HASH_INDEX_T start, HASH_INDEX_T m, const KeyType&) {
        start_ = start; m_ = m; numProbes_ = 0;
    }
    HASH_INDEX_T next() { throw std::logic_error("Base Prober"); }
};

template <typename KeyType>
struct LinearProber : Prober<KeyType> {
    HASH_INDEX_T next() {
        if (this->numProbes_ >= this->m_) return this->npos;
        HASH_INDEX_T loc = (this->start_ + this->numProbes_) % this->m_;
        this->numProbes_++;
        return loc;
    }
};

template <typename KeyType, typename Hash2>
struct DoubleHashProber : Prober<KeyType> {
    Hash2 h2_;
    HASH_INDEX_T dhstep_;
    static const HASH_INDEX_T DOUBLE_HASH_MOD_VALUES[];
    static const int DOUBLE_HASH_MOD_SIZE;

    DoubleHashProber(const Hash2& h2 = Hash2()) : h2_(h2) {}

    void init(HASH_INDEX_T start, HASH_INDEX_T m, const KeyType& key) {
        Prober<KeyType>::init(start, m, key);
        // choose largest modulus < m
        HASH_INDEX_T mod = DOUBLE_HASH_MOD_VALUES[0];
        for (int i = 0; i < DOUBLE_HASH_MOD_SIZE && DOUBLE_HASH_MOD_VALUES[i] < m; ++i)
            mod = DOUBLE_HASH_MOD_VALUES[i];
        dhstep_ = mod - (h2_(key) % mod);
    }

    HASH_INDEX_T next() {
        if (this->numProbes_ >= this->m_) return this->npos;
        HASH_INDEX_T loc = (this->start_ + dhstep_ * this->numProbes_) % this->m_;
        this->numProbes_++;
        return loc;
    }
};

template <typename KeyType, typename Hash2>
const HASH_INDEX_T DoubleHashProber<KeyType,Hash2>::DOUBLE_HASH_MOD_VALUES[] = {
    7, 19, 43, 89, 193, 389, 787, 1583, 3191, 6397,
    12841, 25703, 51431, 102871, 205721, 411503,
    823051, 1646221, 3292463, 6584957, 13169963,
    26339921, 52679927, 105359939, 210719881,
    421439749, 842879563, 1685759113
};
template <typename KeyType, typename Hash2>
const int DoubleHashProber<KeyType,Hash2>::DOUBLE_HASH_MOD_SIZE =
    sizeof(DoubleHashProber<KeyType,Hash2>::DOUBLE_HASH_MOD_VALUES) /
    sizeof(HASH_INDEX_T);

// ---- HashTable ----
template<
    typename K, typename V,
    typename Prober = LinearProber<K>,
    typename Hash = std::hash<K>,
    typename KEqual = std::equal_to<K>
>
class HashTable {
public:
    typedef std::pair<K,V> ItemType;

    struct HashItem {
        ItemType item;
        bool deleted;
        HashItem(const ItemType& p): item(p), deleted(false) {}
    };

    HashTable(double alpha = 0.4,
              const Prober& p = Prober(),
              const Hash& h = Hash(),
              const KEqual& eq = KEqual())
      : resizeAlpha_(alpha),
        hash_(h), kequal_(eq),
        prober_(p), totalProbes_(0),
        mIndex_(0), size_(0), deletedCount_(0)
    {
        table_.assign(CAPACITIES[0], nullptr);
    }

    ~HashTable() {
        for (auto ptr : table_) delete ptr;
    }

    bool empty() const { return size_ == 0; }
    size_t size() const { return size_; }

    void insert(const ItemType& p) {
        // resize if load >= alpha
        double load = double(size_ + deletedCount_) / table_.size();
        if (load >= resizeAlpha_) resize();

        K key = p.first;
        HASH_INDEX_T h0 = hash_(key) % table_.size();
        prober_.init(h0, table_.size(), key);
        HASH_INDEX_T loc = prober_.next(); ++totalProbes_;
        while (loc != Prober::npos) {
            if (table_[loc] == nullptr) {
                table_[loc] = new HashItem(p);
                ++size_;
                return;
            }
            if (!table_[loc]->deleted && kequal_(table_[loc]->item.first, key)) {
                table_[loc]->item.second = p.second;
                return;
            }
            loc = prober_.next(); ++totalProbes_;
        }
        throw std::logic_error("HashTable full");
    }

    void remove(const K& key) {
        HashItem* hi = internalFind(key);
        if (hi && !hi->deleted) {
            hi->deleted = true;
            --size_;
            ++deletedCount_;
        }
    }

    const ItemType* find(const K& key) const {
        HASH_INDEX_T h = probe(key);
        if (h == Prober::npos || table_[h] == nullptr) return nullptr;
        return &table_[h]->item;
    }
    ItemType* find(const K& key) {
        HASH_INDEX_T h = probe(key);
        if (h == Prober::npos || table_[h] == nullptr) return nullptr;
        return &table_[h]->item;
    }

    const V& at(const K& key) const {
        HashItem* hi = internalFind(key);
        if (!hi) throw std::out_of_range("Bad key");
        return hi->item.second;
    }
    V& at(const K& key) {
        HashItem* hi = internalFind(key);
        if (!hi) throw std::out_of_range("Bad key");
        return hi->item.second;
    }

    const V& operator[](const K& key) const { return at(key); }
    V& operator[](const K& key)       { return at(key); }

    void reportAll(std::ostream& out) const {
        for (size_t i = 0; i < table_.size(); ++i) {
            if (table_[i]) {
                out << "Bucket " << i << ": "
                    << table_[i]->item.first << " "
                    << table_[i]->item.second << "\n";
            }
        }
    }

private:
    // locate HashItem* for key (or nullptr)
    HashItem* internalFind(const K& key) const {
        HASH_INDEX_T h = probe(key);
        if (h == Prober::npos || table_[h] == nullptr) return nullptr;
        HashItem* hi = table_[h];
        if (!hi->deleted && kequal_(hi->item.first, key))
            return hi;
        return nullptr;
    }

    // probing to either an empty slot or matching key
    HASH_INDEX_T probe(const K& key) const {
        HASH_INDEX_T h0 = hash_(key) % table_.size();
        prober_.init(h0, table_.size(), key);
        HASH_INDEX_T loc = prober_.next(); ++totalProbes_;
        while (loc != Prober::npos) {
            if (table_[loc] == nullptr) return loc;
            if (!table_[loc]->deleted &&
                 kequal_(table_[loc]->item.first, key))
                return loc;
            loc = prober_.next(); ++totalProbes_;
        }
        return Prober::npos;
    }

    void resize() {
        if (mIndex_ + 1 >= (int)(sizeof(CAPACITIES)/sizeof(CAPACITIES[0])))
            throw std::logic_error("No more capacity");
        ++mIndex_;
        auto oldTable = std::move(table_);
        table_.assign(CAPACITIES[mIndex_], nullptr);
        size_ = 0;
        deletedCount_ = 0;

        for (auto ptr : oldTable) {
            if (ptr) {
                if (!ptr->deleted) {
                    insert(ptr->item);
                }
                delete ptr;
            }
        }
    }

    std::vector<HashItem*> table_;
    double       resizeAlpha_;
    Hash         hash_;
    KEqual       kequal_;
    mutable Prober prober_;
    mutable size_t totalProbes_;

    static const HASH_INDEX_T CAPACITIES[];
    int           mIndex_;
    size_t        size_, deletedCount_;
};

template<typename K,typename V,typename P,typename H,typename E>
const HASH_INDEX_T HashTable<K,V,P,H,E>::CAPACITIES[] = {
    11,23,47,97,197,397,797,1597,3203,6421,12853,25717,51437,102877,
    205759,411527,823117,1646237,3292489,6584983,13169977,26339969,
    52679969,105359969,210719881,421439783,842879579,1685759167
};

#endif