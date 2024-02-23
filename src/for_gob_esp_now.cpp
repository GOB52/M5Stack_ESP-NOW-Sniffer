/*
  gob_esp_now specific output
 */
#include <M5Unified.h>
#include <for_gob_esp_now.hpp>
#include <type_traits>

extern void dump(const uint8_t* buf, const size_t len);

template<typename E> constexpr inline typename std::underlying_type<E>::type to_underlying(const E& e) noexcept
{
    return static_cast<typename std::underlying_type<E>::type>(e);
}

struct RUDP
{
    ///@cond 0
    static constexpr uint8_t _SYN = 0x80; // Begin session
    static constexpr uint8_t _ACK = 0x40; // Acknowledge
    static constexpr uint8_t _EAK = 0x20; // Selective ACK
    static constexpr uint8_t _RST = 0x10; // End session
    static constexpr uint8_t _NUL = 0x08; // Heartbeat
    static constexpr uint8_t _CHK = 0x04; // Calculate checksum include payload?
    static constexpr uint8_t _TCS = 0x02; // Demand for resumption
    ///@endcond
    
    /*!
      @enum Flag
      @brief Flags of RUDP
     */
    enum class Flag : uint8_t
     {
         NONE    = 0,           //!< @brief Unreliable
         SYN     = _SYN,        //!< @brief Begin session
         SYN_ACK = _SYN | _ACK, //!< @brief Begin session with ACK
         ACK     = _ACK,        //!< @brief Acknowledge (with payload if exists)
         //ACK_CHK = _ACK | _CHK,
         //EAK     = _ACK | _EAK,
         RST     = _RST,        //!< @brief End session
         RST_ACK = _ACK | _RST, //!< @brief End session with ACK
         NUL     = _NUL | _ACK, //!< @brief Heartbeat
         //TCS = _TCS,
         //TCS_ACK = _TCS | _ACK,
     };
    using flag_t = std::underlying_type<Flag>::type; //!< @brief Flag type
    static_assert(sizeof(flag_t) == sizeof(uint8_t), "Illegal size");
    
    flag_t  flag{};     //!< @brief flag ==0:Unreliable !=0:RUDP flags
    uint8_t sequence{}; //!< @brief Lower 8bit of sequence
    uint8_t ack{};      //!< @brief Lower 8bit of ack
    uint8_t _sum{};     //!< @brief Check sum (Not supported yet...)

    ///@name Default config values
    ///@{
    static constexpr uint16_t DEFAULT_RETRANSMISSION_TIMEOUT = (320);
    static constexpr uint16_t DEFAULT_CUMULATIVE_ACK_TIMEOUT = (100);
    static constexpr uint16_t DEFAULT_NULL_TIMEOUT = (1000*10);
    static constexpr uint16_t DEFAULT_TRANSFER_STATE_TIMEOUT = 0;
    static constexpr uint8_t DEFAULT_MAX_RETRANS = 8;
    static constexpr uint8_t DEFAULT_MAX_CUM_ACK = 4;
    static constexpr uint8_t DEFAULT_MAX_OUT_OF_SEQ = 0;
    static constexpr uint8_t DEFAULT_MAX_AUTO_RESET = 0;
    ///@}

    /*!
      @struct config_t
      @brief SYN parameters
      @warning Note that not all are supported
      @sa https://datatracker.ietf.org/doc/html/draft-ietf-sigtran-reliable-udp-00
     */
    struct config_t
    {
        uint8_t session{};     //!< @brief  Session id
        uint8_t outstanding{}; //!< @brief  The maximum number of segments that should be sent without getting an acknowledgment. (as window size)
        uint8_t maximumSegmentSize{250}; //!< @brief The maximum number of octets that can be received by the peer (Fixed ESP_NOW_MAX_DATA_LEN in this library)
        //! @brief The timeout value for retransmission of unacknowledged packets
        uint16_t retransmissionTimeout{DEFAULT_RETRANSMISSION_TIMEOUT};
        //! @brief The timeout value for sending an acknowledgment segment if another segment is not sent
        uint16_t cumulativeAckTimeout{DEFAULT_CUMULATIVE_ACK_TIMEOUT};
        //! @brief The timeout value for sending a null segment if a data segment has not been sent
        uint16_t nullSegmentTimeout{DEFAULT_NULL_TIMEOUT};
        //! @brief This timeout value indicate the amount of time the state information will be  saved for a connection before an auto reset occurs.        
        uint16_t transferStateTimeout{DEFAULT_TRANSFER_STATE_TIMEOUT};
        //! @brief The maximum number of times consecutive retransmission(s)
        uint8_t maxRetrans{DEFAULT_MAX_RETRANS};
        //! @brief The maximum number of acknowledgments that will be accumulated before sending an acknowledgment
        uint8_t maxCumAck{DEFAULT_MAX_CUM_ACK};
        //! @brief he maximum number of out of sequence packets that will be accumulated before an EACK segment is sent
        uint8_t maxOutOfSeq{DEFAULT_MAX_OUT_OF_SEQ};
        //! @brief The maximum number of consecutive auto reset that will performed before a connection is reset
        uint8_t maxAutoReset{DEFAULT_MAX_AUTO_RESET};
    }  __attribute__((__packed__));

}  __attribute__((__packed__));



struct TransceiverHeader
{
    uint8_t tid{};  //!< @brief Identifier
    uint8_t size{}; //!< @brief Data size including this header
    RUDP    rudp{}; //!< @brief RUDP header
    // Payload continues if exists

    ///@name Properties
    ///@{
    inline bool isRUDP() const       { return (bool)rudp.flag; } //!< @brief is RUDP data?
    inline bool isSYN() const        { return (rudp.flag & to_underlying(RUDP::Flag::SYN)); } //!< @brief is SYN ?
    inline bool onlySYN() const      { return (rudp.flag == to_underlying(RUDP::Flag::SYN)); } //!< @brief only SYN?
    //inline bool isSYN() const        { return (rudp.flag == to_underlying(RUDP::Flag::SYN)); } //!< @brief is SYN ?

    inline bool isRST() const        { return (rudp.flag & to_underlying(RUDP::Flag::RST)); } //!< @brief is RST?
    inline bool isACK() const        { return (rudp.flag & to_underlying(RUDP::Flag::ACK)); } //!< @brief is ACK?
    inline bool onlyACK() const      { return (rudp.flag == to_underlying(RUDP::Flag::ACK)); } //!< @brief only ACK?
    inline bool isNUL() const        { return (rudp.flag == to_underlying(RUDP::Flag::NUL)); } //!< @brief is NUL?
    inline bool hasPayload() const   { return size > sizeof(*this); } //!< @brief Has payload?
    inline uint8_t payloadSize() const { return hasPayload() ? size - sizeof(*this) : 0; } //!< @brief payload size if exists
    inline uint8_t* payload() const  { return hasPayload() ? ((uint8_t*)this) + sizeof(*this) : nullptr; } //!< @brief Gets the payload pointer if exists.
    inline bool needReturnACK() const { return (isACK() && payload()) || isNUL() || (isRST() && isACK()); }
    ///@}
}  __attribute__((__packed__));

struct CommunicatorHeader
{
    static constexpr uint16_t SIGNETURE = 0x5200; //!< @brief Packet signeture(8bit) + Header version(8bit)
    uint16_t signeture{SIGNETURE}; //!< @brief Signeture of the Communicator's data
    uint8_t  app_id{};             //!< @brief Application-specific ID
    uint8_t  count{};              //!< @brief Number of transceiver data
    uint8_t  size{sizeof(*this)};  //!< @brief Size of packet [ |CH|TH...|TH......|TH.| ]
    // Transceiver data continues for count times.
}  __attribute__((__packed__));

bool is_gob_esp_now_packet(const uint8_t* payload, const size_t length)
{
    return length >= sizeof(CommunicatorHeader)
            && ((const CommunicatorHeader*)payload)->signeture == CommunicatorHeader::SIGNETURE
            && ((const CommunicatorHeader*)payload)->size >= sizeof(CommunicatorHeader);
}

void output_gob_esp_now_payload(const uint8_t* payload)
{
    auto ch = (const CommunicatorHeader*)payload;

    M5.Log.printf("CH %u:%u\n\r", ch->app_id, ch->count);
    int cnt = ch->count;
    auto ptr = payload + sizeof(CommunicatorHeader);
    while(cnt--)
    {
        auto th = (const TransceiverHeader*)ptr;
        M5.Log.printf("-- TH %u:0x%02X:%u:%u\n\r", th->tid, th->rudp.flag, th->rudp.sequence, th->rudp.ack);
        dump(th->payload(), th->payloadSize());
        ptr += th->size;
    }
}
