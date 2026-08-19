#include <iostream>
#include <vector>
#include <queue>
#include <climits>
#include <algorithm>

struct Record
{
    uint64_t timestamp;
    uint64_t streamId;
    long long data;

    void dump() const
    {
        std::cout << "ts:" << timestamp << " " << "streamId:" << streamId << " " << "data:" << data << "\n";
    }
};

struct RecordCmp
{
    bool operator()(const Record& r1, const Record& r2) const
    {
        return r1.timestamp > r2.timestamp;
    }
};

class StreamMerger
{
    private:
        uint64_t L;
        uint64_t maxTsSeen;
        uint64_t watermark;

        std::priority_queue<Record, std::vector<Record>, RecordCmp> pq;
        std::vector<Record> mergedRecords;

    public:

        StreamMerger(int lateness): L(lateness), maxTsSeen(0), watermark(0)
        {
        }

        void addRecord(const Record& record)
        {
            // Don't allow older records
            if(record.timestamp < watermark)
            {
                return;
            }

            pq.push(record);

            // update maxTsSeen, watermark
            maxTsSeen = std::max(maxTsSeen, record.timestamp);
            watermark = maxTsSeen - L;

            // pop all records whose ts < watermark and push
            // into mergedRecords (records safe to emit)
            while(!pq.empty() && pq.top().timestamp <= watermark)
            {
                mergedRecords.push_back(pq.top());
                pq.pop();
            }
        }

        void flushAll()
        {
            while(!pq.empty())
            {
                mergedRecords.push_back(pq.top());
                pq.pop();
            }
        }

        std::vector<Record> getMergedRecords() const
        {
            return mergedRecords;
        }

};

int main()
{
    int lateness = 5;
    StreamMerger streamMerger(lateness);

    streamMerger.addRecord({10,0,100});
    streamMerger.addRecord({7,1,300});
    streamMerger.addRecord({15,0,100});
    streamMerger.addRecord({12,1,400});
    streamMerger.addRecord({14,2,500});

    streamMerger.flushAll();

    for(const auto& record: streamMerger.getMergedRecords())
    {
        record.dump();
    }

  return 0;
}
