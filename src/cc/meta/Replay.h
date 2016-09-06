/*!
 * $Id$
 *
 * \file Replay.h
 * \brief log replay definitions
 * \author Blake Lewis (Kosmix Corp.)
 *
 * Copyright 2008-2012 Quantcast Corp.
 * Copyright 2006-2008 Kosmix Corp.
 *
 * This file is part of Kosmos File System (KFS).
 *
 * Licensed under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#if !defined(KFS_REPLAY_H)
#define KFS_REPLAY_H

#include "MetaVrLogSeq.h"

#include "common/kfstypes.h"
#include "common/MdStream.h"

#include <string>
#include <istream>
#include <fstream>

namespace KFS
{
using std::string;
using std::ifstream;
using std::istream;

class DETokenizer;
class DiskEntry;
class MetaVrLogStartView;
struct MetaRequest;

class Replay
{
public:
    bool verifyLogSegmentsPresent()
    {
        lastLogNum = -1;
        return getLastLogNum() == 0;
    }
    //!< open the log file for replay
    int openlog(const string &p);
    int logno() { return number; }
    //!< read and apply its contents
    int playLogs(bool includeLastLogFlag = false);
    //!< starting from log for logno(),
    //!< replay all logs we have in the logdir.
    int playAllLogs() { return playLogs(true); }
    bool getAppendToLastLogFlag() const { return appendToLastLogFlag; }
    int getLastLogIntBase() const { return lastLogIntBase; }
    inline void setRollSeeds(int64_t roll);
    int64_t getRollSeeds() const { return rollSeeds; }
    int64_t getErrChksum() const
        { return errChecksum; }
    MetaVrLogSeq getCommitted() const
        { return committed; }
    void setErrChksum(int64_t sum)
        { errChecksum = sum; }
    void setCommitted(const MetaVrLogSeq& seq)
    {
        checkpointCommitted = seq;
        committed           = seq;
        lastLogStart        = seq;
        lastLogSeq          = seq;
    }
    const string& getCurLog() const
        { return path; }
    string getLastLogName() const
    {
        if (0 <= lastLogNum) {
            return const_cast<Replay*>(this)->logfile(lastLogNum);
        }
        return string();
    }
    int getLastCommittedStatus() const
        { return lastCommittedStatus; }
    MdStateCtx getMdState() const
        { return mds.GetMdState(); }
    seq_t getLogNum() const
        { return number; }
    MetaVrLogSeq getLastLogStart() const
        { return lastLogStart; }
    MetaVrLogSeq getLastLogSeq() const
        { return lastLogSeq; }
    seq_t getLastBlockSeq() const
        { return lastBlockSeq; }
    int playLine(const char* line, int len, seq_t blockSeq);
    bool logSegmentHasLogSeq() const
        { return logSegmentHasLogSeq(number); }
    void verifyAllLogSegmentsPreset(bool flag)
        { verifyAllLogSegmentsPresetFlag = flag; }
    void setLogDir(const char* dir);
    MetaVrLogSeq getCheckpointCommitted() const
        { return checkpointCommitted; }
    void handle(MetaVrLogStartView& op);
    void setReplayState(
        const MetaVrLogSeq& committed,
        seq_t               seed,
        int64_t             status,
        int64_t             errChecksum,
        MetaRequest*        commitQueue);
    bool runCommitQueue(
        const MetaVrLogSeq& committed,
        seq_t               seed,
        int64_t             status,
        int64_t             errChecksum);
    bool commitAll();
    bool submit(MetaRequest& req)
        { return (enqueueFlag && enqueue(req)); }
    vrNodeId_t getPrimaryNodeId() const
        { return primaryNodeId; }

    class BlockChecksum
    {
    public:
        BlockChecksum();
        uint32_t blockEnd(size_t skip);
        bool write(const char* buf, size_t len);
        bool flush() { return true; }
    private:
        size_t   skip;
        uint32_t checksum;
    };
    class State;
    class Tokenizer
    {
    public:
        Tokenizer(istream& file, Replay* replay, bool* enqueueFlag);
        ~Tokenizer();
        DETokenizer& Get() { return tokenizer; }
        State& GetState() { return state; }
    private:
        State&       state;
        DETokenizer& tokenizer;
    };
    static void AddRestotreEntries(DiskEntry& e);
private:
    typedef MdStreamT<BlockChecksum> MdStream;

    ifstream         file;   //!< the log file being replayed
    string           path;   //!< path name for log file
    seq_t            number; //!< sequence number for log file
    seq_t            lastLogNum;
    int              lastLogIntBase;
    bool             appendToLastLogFlag;
    bool             verifyAllLogSegmentsPresetFlag;
    bool             enqueueFlag;
    Tokenizer        replayTokenizer;
    MetaVrLogSeq&    checkpointCommitted;
    MetaVrLogSeq&    committed;
    MetaVrLogSeq     lastLogStart;
    MetaVrLogSeq&    lastLogSeq;
    seq_t&           lastBlockSeq;
    int64_t&         errChecksum;
    int64_t          rollSeeds;
    int              lastCommittedStatus;
    size_t           tmplogprefixlen;
    string           tmplogname;
    string           logdir;
    MdStream         mds;
    const DiskEntry& entrymap;
    BlockChecksum    blockChecksum;
    seq_t            maxLogNum;
    seq_t            logSeqStartNum;
    vrNodeId_t       primaryNodeId;

    friend class MetaServerGlobals;
    Replay();
    ~Replay();
    int playLogs(seq_t lastlog, bool includeLastLogFlag);
    int playlog(bool& lastEntryChecksumFlag);
    int getLastLogNum();
    const string& logfile(seq_t num);
    bool logSegmentHasLogSeq(seq_t num) const
        { return  (0 <= logSeqStartNum && logSeqStartNum <= num); }
    void update();
    string getLastLog();
    bool enqueue(MetaRequest& req);
private:
    // No copy.
    Replay(const Replay&);
    Replay& operator=(const Replay&);
};

bool restore_chunk_server_end(DETokenizer& c);
extern Replay& replayer;

}
#endif // !defined(KFS_REPLAY_H)
