#define PACKET_HANDSHAKE_SERVERBOUND_HANDSHAKE 0x00;

struct mcpr_packet {
    int8_t id;
    union {
        union {

        } handshake_clientbound;

        union {
            struct {
                int32_t protocol_version;
                char *server_address;
                uint16_t server_port;
                enum mcpr_state next_state;
            } handshake;
        } handshake_serverbound;

        union {

        } play_clientbound;

        union {

        } play_serverbound;

        union {

        } status_clientbound;

        union {
            struct {

            } request;

            struct {
                int64_t payload;
            } ping;
        } status_serverbound;

        union {

        } login_clientbound;

        union {

        } login_serverbound;

    } data;
};
