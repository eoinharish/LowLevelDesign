#include <iostream>
#include <vector>
#include <cstdint>
#include <optional>
#include <concepts>

using namespace std;

template<typename T>
concept Printable = requires(std::ostream& os, const T& x)
{
    os << x;
};

template<typename K, typename V>
class FlatMap
{
    private:
        struct Slot
        {
            K key;
            V value;
        };

        enum class State : uint8_t
        {
            EMPTY, FULL, DELETED
        };

        size_t capacity;
        vector<Slot> slots; // contiguous K,V storage
        vector<State> ctrl; // 1 control byte of metadata per slot
        size_t tombstone = 0;
        size_t live = 0; // real data

        size_t hash(K key)
        {
            size_t h = std::hash<K>()(key);
            return h & (capacity - 1); // capacity is a power of 2
        }

        void rehash(size_t newCapacity)
        {
            cout << "Rehashing...\n";
            vector<Slot> newSlots(newCapacity);
            vector<State> newCtrl(newCapacity, State::EMPTY);

            size_t old_capacity = capacity;
            capacity = newCapacity; // IMP: probing now use the new size

            for (size_t i=0; i < old_capacity; i++)
            {
                if (ctrl[i] != State::FULL) continue; // skip EMPTY and DELETED

                size_t index = hash(slots[i].key);

                // IMP : Need probing as indexes might collide
                while (newCtrl[index] == State::FULL)
                {
                    index = (index + 1) % capacity; // (index + 1) & (capacity - 1)
                }

                newSlots[index] = std::move(slots[i]);
                newCtrl[index] = State::FULL;
            }

            slots = std::move(newSlots);
            ctrl = std::move(newCtrl);
            tombstone = 0;
        }

    public:

        FlatMap(size_t cap = 8) : capacity(cap), slots(cap), ctrl(cap, State::EMPTY)
        {
        }

        bool insert(const K& key, const V& value)
        {
            // Check load factor
            // Tombstones count toward load, or a table with no EMPTY slot loops forever.
            if ((live + tombstone) * 4 >= capacity * 3)
            {
                if (tombstone < live)
                {
                    rehash(capacity * 2); // mostly real data, grow (double the size)
                }
                else
                {
                    rehash(capacity); // mostly tombstones, clean up (rehash with same size)
                }
            }

            size_t index = hash(key);
            size_t firstDeleted = SIZE_MAX;

            // Find first available slot
            while (ctrl[index] != State::EMPTY)
            {
                if (ctrl[index] == State::FULL && slots[index].key == key)
                {
                    slots[index].value = value;
                    return false;
                }
                else if (ctrl[index] == State::DELETED && firstDeleted == SIZE_MAX)
                {
                    firstDeleted = index; // keep scanning for duplicates, key might be present after this deleted slot
                }

                index = (index + 1) & (capacity - 1);
            }

            if (firstDeleted != SIZE_MAX)
            {
                // tombstone index will be used
                index = firstDeleted;
                tombstone--;
            }

            slots[index] = {key,value};
            ctrl[index] = State::FULL;
            live++;

            return true;
        }

        std::optional<V> get(const K& key)
        {
            size_t index = hash(key);

            // only EMPTY ends a probe chain
            while(ctrl[index] != State::EMPTY)
            {
                if (ctrl[index] == State::FULL && slots[index].key == key)
                {
                    return slots[index].value;
                }
                index = (index + 1) & (capacity - 1);
            }
            return nullopt;
        }

        // find slot for the key
        size_t find(const K& key)
        {
            size_t index = hash(key);

            while (ctrl[index] != State::EMPTY)
            {
                if (ctrl[index] == State::FULL && slots[index].key == key)
                {
                    return index;
                }
                index = (index + 1) & (capacity - 1);
            }
            return SIZE_MAX;
        }

        bool erase(const K& key)
        {
            size_t index = find(key); // find the slot for the Key

            if (index == SIZE_MAX)  return false;

            ctrl[index] = State::DELETED; // mark tombstone
            tombstone++;
            live--;

            return true;
        }

        size_t size() const
        {
            return live;
        }

        void printMap() const requires Printable<K> && Printable<V>
        {
            for(size_t i=0; i<capacity; i++)
            {
                if (ctrl[i] == State::FULL)
                {
                    cout << "K: " << slots[i].key << ", V: " << slots[i].value << " -> State: "
                        << static_cast<int>(ctrl[i]) << endl;
                }
            }
        }

};

int main()
{
    FlatMap<int, string> mpp;
    
    for(int i=0; i<5; i++)
    {
        if (mpp.insert(i, "harish") == false)
        {
            cout << "Cannot insert\n";
        }
    }

    mpp.erase(4);
    mpp.printMap();
    mpp.insert(6, "uday");

    if (auto ptr = mpp.get(6))
    {
        cout << *ptr << endl;
    }

    //mpp.erase(6);
    mpp.printMap();
    mpp.insert(17, "uday");

    return 0;
}