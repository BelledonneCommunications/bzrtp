/*
 * Copyright (c) 2014-2019 Belledonne Communications SARL.
 *
 * This file is part of bzrtp.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef TYPEDEF_H
#define TYPEDEF_H

/* maximum number of simultaneous channels opened in a ZRTP session */
#define ZRTP_MAX_CHANNEL_NUMBER 64
/* aux secret may rarely be used define his maximum length in bytes */
#define MAX_AUX_SECRET_LENGTH	64
/* the context will store some of the sent or received packets */
#define PACKET_STORAGE_CAPACITY 5

/* HELLO_MESSAGE_STORE_ID MUST stay at 0 in case of we are treating a GoClear message and we are deleting all messages except Hello Packets */
#define HELLO_MESSAGE_STORE_ID      0	/* MUST stay at 0 */
#define	COMMIT_MESSAGE_STORE_ID     1
#define DHPART_MESSAGE_STORE_ID     2
#define CONFIRM_MESSAGE_STORE_ID    3
#define GOCLEAR_MESSAGE_STORE_ID    4

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>


#ifdef ZIDCACHE_ENABLED
#include "sqlite3.h"
#endif /* ZIDCACHE_ENABLED */

typedef struct bzrtpChannelContext_struct bzrtpChannelContext_t;

#include <bctoolbox/crypto.h>
#include <bctoolbox/port.h>
#include "packetParser.h"
#include "stateMachine.h"

/* logging */
/* log domain is defined in CMakeList */
#include "bctoolbox/logging.h"

#ifdef _WIN32
#define snprintf _snprintf
#endif

/* timer related definitions */
#define BZRTP_TIMER_ON 1
#define BZRTP_TIMER_OFF 2

/* values for retransmission timers, as recommended in rfc section 6 */
#define HELLO_BASE_RETRANSMISSION_STEP 	50
#define HELLO_CAP_RETRANSMISSION_STEP 	200
#define HELLO_MAX_RETRANSMISSION_NUMBER	20

#define NON_HELLO_BASE_RETRANSMISSION_STEP 	150
#define NON_HELLO_CAP_RETRANSMISSION_STEP 	1200
#define NON_HELLO_MAX_RETRANSMISSION_NUMBER	10

#define CLEARACK_BASE_RETRANSMISSION_STEP    5000
#define CLEARACK_MAX_RETRANSMISSION_NUMBER   20

/* network related defines */
/* minimal MTU size is 600 bytes to avoid useless fragmentation of small enough packets */
#define BZRTP_MINIMUM_MTU 600
/* default MTU is 1452 to aim at 1500 bytes with IPv6(40 bytes) + UDP(8 bytes) overhead */
#define BZRTP_DEFAULT_MTU 1452

/* Client identifier can contain up to 16 characters, it identify the BZRTP library version */
/* Use it to pass bzrtp version number to peer, is it part of Hello message */
/* custom Linphone Instant Messaging Encryption depends on bzrtp version */
/* Note: ZRTP_VERSION and BZrtp version are for now both at 1.1 but they are unrelated */
/* historically since the creation of bzrtp, it used client idenfiers : */
#define ZRTP_CLIENT_IDENTIFIERv1_0a "LINPHONE-ZRTPCPP"
#define ZRTP_CLIENT_IDENTIFIERv1_0b "BZRTP"
/* Since version 1.1 which implement correctly the key export mechanism described in ZRTP RFC 4.5.2, bzrtp lib identifies itself as */
#define ZRTP_CLIENT_IDENTIFIERv1_1 "BZRTPv1.1"

#define ZRTP_CLIENT_IDENTIFIER ZRTP_CLIENT_IDENTIFIERv1_1

#ifdef __cplusplus
extern "C"{
#endif

/* pgp word list for use with SAS */
extern const char * pgpWordsEven[];
extern const char * pgpWordsOdd[];

/**
 * @brief Timer structure : The timer mechanism receives a tick giving a current time in ms
 * a timer object will check on tick reception if it must fire or not
 */
typedef struct bzrtpTimer_struct {
	uint8_t status; /**< Status is BZRTP_TIMER_ON or BZRTP_TIMER_OFF */
	uint64_t firingTime; /**< in ms. The timer will fire if currentTime >= firingTime */
	uint8_t firingCount; /**< Timer is used to resend packets, count the number of times a packet has been resent */
	int timerStep; /**< in ms. Step between next timer fire: used to reset firingTime for next timer fire */
} bzrtpTimer_t;

/* the rs1 and rs2 are 256 bits long - see rfc section 4.6.1 */
#define RETAINED_SECRET_LENGTH 32
/**
 * @brief A set of cached secrets retrieved from the cache as defined
 */
typedef struct cachedSecrets_struct {
	uint8_t *rs1; /**< retained secret 1 */
	uint8_t rs1Length; /**< retained secret 1 length in bytes */
	uint8_t *rs2; /**< retained secret 2 */
	uint8_t rs2Length; /**< retained secret 2 length in bytes */
	uint8_t *auxsecret; /**< auxiliary secret */
	size_t auxsecretLength; /**< auxiliary secret length in bytes */
	uint8_t *pbxsecret; /**< PBX secret */
	size_t pbxsecretLength; /**< PBX secret length in bytes */
	uint8_t previouslyVerifiedSas; /* boolean, is a SAS has been previously verified with this user */
} cachedSecrets_t;

/**
 * @brief The hash of cached secret truncated to the 64 leftmost bits
 * aux secret ID is not part of it because channel context dependend while these one are session wise
 */
typedef struct cachedSecretsHash_struct {
	uint8_t rs1ID[8]; /**< retained secret 1 Hash */
	uint8_t rs2ID[8]; /**< retained secret 2 Hash */
	uint8_t pbxsecretID[8]; /**< pbx secret Hash */
} cachedSecretsHash_t;

typedef struct fragmentInfo_struct {
	uint16_t offset;
	uint16_t length;
} fragmentInfo_t;

typedef struct fragmentReassembly_struct {
	uint16_t messageId; /**< message ID of the current message */
	uint16_t messageLength; /**< total length (in 32 bits words) of the current message */
	uint8_t *packetString; /**< Storage for the packet - it includes the not used packet header */
	bctbx_list_t *fragments; /**< list of fragmentInfo_t on all what we already have */
} fragmentReassembly_t;
/**
 * @brief The zrtp context of a channel
 *
 */
struct bzrtpChannelContext_struct {

	void *clientData; /**< this is a pointer provided by the client which is then resent as a parameter of the callbacks functions. Usefull to store RTP session context for example */

	uint8_t role;/**< can be INITIATOR or RESPONDER, is set to INITIATOR at creation, may switch to responder later */
	bzrtpStateMachine_t stateMachine; /**< The state machine function, holds the current state of the channel: points to the current state function */
	bzrtpTimer_t timer; /**< a timer used to manage packets retransmission */

	uint32_t selfSSRC; /**< A context is identified by his own SSRC and the peer one */

	/* flags */
	uint8_t isSecure; /**< This flag is set to 1 when the ZRTP negociation ends and SRTP secrets are generated and confirmed for this channel */
	uint8_t isMainChannel; /**< This flag is set for the firt channel only, allow to distinguish channel to be secured using DHM or multiStream */
#ifdef GOCLEAR_ENABLED
	uint8_t isClear; /**< This flag is set to 1 when this channel is in clear state */
	uint8_t hasReceivedAGoClear; /**< This flag is set to 1 when this channel has received a GoClear message */
#endif

	/* Hash chains, self is generated at channel context init */
	uint8_t selfH[4][32]; /**< Store self 256 bits Hash images H0-H3 used to generate messages MAC */
	uint8_t peerH[4][32]; /**< Store peer 256 bits Hash images H0-H3 used to check messages authenticity */

	/* packet storage : shall store some sent and received packets */
	bzrtpPacket_t *selfPackets[PACKET_STORAGE_CAPACITY]; /**< Hello, Commit and DHPart packet locally generated */
	bzrtpPacket_t *peerPackets[PACKET_STORAGE_CAPACITY]; /**< Hello, Commit and DHPart packet received from peer */

	/* peer Hello hash : store the peer hello hash when given by signaling */
	uint8_t *peerHelloHash; /**< peer hello hash - SHA256 of peer Hello packet, given through signaling, shall be a 32 bytes buffer */

	/* sequence number: self and peer */
	uint16_t selfSequenceNumber; /**< Sequence number of the next packet to be sent */
	uint16_t selfMessageSequenceNumber; /**< is used as messageId for fragmented packets, is incremented on new message creation, not packet sending */
	uint16_t peerSequenceNumber; /**< Sequence number of the last valid received packet */

	/* algorithm agreed after Hello message exchange(use mapping define in cryptoUtils.h) and the function pointer to use them */
	uint8_t hashAlgo; /**< hash algorithm agreed on after Hello packet exchange, stored using integer mapping defined in cryptoUtils.h */
	uint8_t hashLength; /**< the length in bytes of a hash generated with the agreed hash algo */
	uint8_t cipherAlgo; /**< cipher algorithm agreed on after Hello packet exchange, stored using integer mapping defined in cryptoUtils.h */
	uint8_t cipherKeyLength; /**< the length in bytes of the key needed by the agreed cipher block algo */
	uint8_t authTagAlgo; /**< srtp authentication tag algorithm agreed on after Hello packet exchange, stored using integer mapping defined in cryptoUtils.h */
	uint8_t keyAgreementAlgo; /**< key agreement algorithm agreed on after Hello packet exchange, stored using integer mapping defined in cryptoUtils.h */
	uint8_t sasAlgo; /**< sas rendering algorithm agreed on after Hello packet exchange, stored using integer mapping defined in cryptoUtils.h */
	uint8_t sasLength; /**< length of the SAS depends on the algorithm agreed */

	/* function pointer to the agreed algorithms - Note, key agreement manage directly this selection so it is not set here */
	void (*hmacFunction)(const uint8_t *key, size_t keyLength, const uint8_t *input, size_t inputLength, uint8_t hmacLength, uint8_t *output); /**< function pointer to the agreed hmacFunction */
	void (*hashFunction)(const uint8_t *input, size_t inputLength, uint8_t hashLength, uint8_t *output); /**< function pointer to the agreed hash function */
	void (*cipherEncryptionFunction)(const uint8_t *key, const uint8_t *IV, const uint8_t *input, size_t inputLength, uint8_t *output); /**< function pointer to the agreed cipher block function, encryption mode */
	void (*cipherDecryptionFunction)(const uint8_t *key, const uint8_t *IV, const uint8_t *input, size_t inputLength, uint8_t *output); /**< function pointer to the agreed cipher block function, decryption mode */
	void (*sasFunction)(uint32_t sas, char * output, int outputSize); /**< function pointer to the agreed sas rendering function */

	/* keys */
	uint8_t *s0; /**< the s0 as describred rfc section 4.4 - have a length of hashLength */
	uint8_t *KDFContext; /**< defined in rfc section 4.4 */
	uint16_t KDFContextLength; /**< length of the KDF context, is 24 + output length of the selected hash algo */
	uint8_t *mackeyi; /**< the initiator mackey as defined in rfc section 4.5.3 - have a length of hashLength */
	uint8_t *mackeyr; /**< the responder mackey as defined in rfc section 4.5.3 - have a length of hashLength*/
	uint8_t *zrtpkeyi; /**< the initiator mackey as defined in rfc section 4.5.3 - have a length of cipherKeyLength */
	uint8_t *zrtpkeyr; /**< the responder mackey as defined in rfc section 4.5.3 - have a length of cipherKeyLength*/
	bzrtpSrtpSecrets_t srtpSecrets; /**< the secrets keys and salt needed by SRTP */

	/* shared secret hash : unlike pbx, rs1 and rs2 secret hash, the auxsecret hash use a channel dependent data (H3) and is then stored in the channel context */
	uint8_t initiatorAuxsecretID[8]; /**< initiator auxiliary secret Hash */
	uint8_t responderAuxsecretID[8]; /**< responder auxiliary secret Hash */

	/* packet fragmentation management */
	/* We do not need to store more than one as there on no scenarii in wich we expect peer to send 2 messages in a parallel */
	fragmentReassembly_t incomingFragmentedPacket;

	/* temporary buffer stored in the channel context */
	bzrtpPacket_t *pingPacket; /**< Temporary stores a ping packet when received to be used to create the pingACK response */

};

/**
 * @brief structure of the ZRTP engine context
 * Store current state, timers, HMAC and encryption keys
 */
struct bzrtpContext_struct {
	/* contexts */
	bctbx_rng_context_t *RNGContext; /**< context for random number generation */
	void *keyAgreementContext; /**< context for the key agreement operations. Only one key agreement computation may be done during a call, so this belongs to the general context and not the channel one */
	uint8_t keyAgreementAlgo; /**< key agreement algorithm agreed on the first channel, the one performing key exchange, stored using integer mapping defined in cryptoUtils.h,  */

	/* flags */
	uint8_t isInitialised; /**< this flag is set once the context was initialised : self ZID retrieved from cache or generated, used to unlock the creation of addtional channels */
	uint8_t isSecure; /**< this flag is set to 1 after the first channel have completed the ZRTP protocol exchange(i.e. when the responder have sent the conf2ACK message), must be set in order to start an additional channel */
	uint8_t peerSupportMultiChannel; /**< this flag is set to 1 when the first valid HELLO packet from peer arrives if it support Multichannel ZRTP */
#ifdef GOCLEAR_ENABLED
	uint8_t selfAcceptGoClear; /**< Set to 1 if this context support receiving GoClear messages */
	uint8_t peerAcceptGoClear; /**< Set to 1 if peer context support receiving GoClear messages */
#endif
	uint64_t timeReference; /**< in ms. This field will set at each channel State Machine start and updated at each tick after creation of the context, it is used to set the firing time of a channel timer */

	/* callbacks */
	bzrtpCallbacks_t zrtpCallbacks; /**< structure holding all the pointers to callbacks functions needed by the ZRTP engine. Functions are set by client using the bzrtp_setCallback function */

	/* channel contexts */
	bzrtpChannelContext_t *channelContext[ZRTP_MAX_CHANNEL_NUMBER]; /**< All the context data needed for a channel are stored in a dedicated structure */

	/* List of available algorithms, initialised with algo implemented in cryptoWrapper but can be then be modified according to user settings */
	uint8_t hc; /**< hash count -zrtpPacket set to 0 means we support only HMAC-SHA256 (4 bits) */
	uint8_t supportedHash[7]; /**< list of supported hash algorithms mapped to uint8_t */
	uint8_t cc; /**< cipher count - set to 0 means we support only AES128-CFB128 (4 bits) */
	uint8_t supportedCipher[7]; /**< list of supported cipher algorithms mapped to uint8_t */
	uint8_t ac; /**< auth tag count - set to 0 mean we support only HMAC-SHA1-32 (4 bits) */
	uint8_t supportedAuthTag[7]; /**< list of supported SRTP authentication tag algorithms mapped to uint8_t */
	uint8_t kc; /**< key agreement count - set to 0 means we support only Diffie-Hellman-Merkle 3072 (4 bits) */
	uint8_t supportedKeyAgreement[7]; /**< list of supported key agreement algorithms mapped to uint8_t */
	uint8_t sc; /**< sas count - set to 0 means we support only base32 (4 bits) */
	uint8_t supportedSas[7]; /**< list of supported Sas representations mapped to uint8_t */

	/* ZIDs and cache */
#ifdef ZIDCACHE_ENABLED
	sqlite3 *zidCache; /**< an sqlite3 db pointer to the zid cache **/
#else
	void *zidCache; /**< an empty pointer always set to NULL when cache is disabled **/
#endif /* ZIDCACHE_ENABLED */
	bctbx_mutex_t *zidCacheMutex; /**< lock access to the cache if provided **/
	int zuid; /**< internal id used to address zid cache SIP/ZID pair binding **/
	char *selfURI; /**< a null terminated string storing the local user URI **/
	uint8_t selfZID[12]; /**< The ZRTP Identifier of this ZRTP end point - a random if running cache less */
	char *peerURI; /**< a null terminated string storing the peer user URI **/
	uint8_t peerZID[12]; /**< The ZRTP Identifier of the peer ZRTP end point - given by the Hello packet */
	uint32_t peerBzrtpVersion; /**< The Bzrtp library version used by peer, retrieved from the peer Hello packet Client identifier and used for backward compatibility in exported key computation */
	cachedSecrets_t cachedSecret; /**< the local cached secrets */
	cachedSecretsHash_t initiatorCachedSecretHash; /**< The hash of cached secret from initiator side, computed as described in rfc section 4.3.1 */
	cachedSecretsHash_t responderCachedSecretHash; /**< The hash of cached secret from responder side, computed as described in rfc section 4.3.1 */
	uint8_t cacheMismatchFlag; /**< Flag set in case of cache mismatch(detected in DHM mode when DH part packet arrives) */
	uint8_t peerPVS; /**< used to store value of PVS flag sent by peer in the confirm packet on first channel only, then used to compute the PVS value sent to the application */

	/* transient auxiliary shared secret : in addition to the auxiliary shared secret stored in ZID cache, caller can provide a shared secret to the zrtp context which will be used for this transaction only */
	/* both auxiliary secret are used and combined as transientAuxiliarySecret appended to cachedAuxiliarySecret*/
	uint8_t *transientAuxSecret; /**< an auxiliary secret not stored in cache, provided after context creation and before the main channel is started */
	size_t transientAuxSecretLength; /**< size of the previous buffer */

	/* keys */
	uint8_t *ZRTPSess; /**< ZRTP session key as described in rfc section 4.5.2 */
	uint8_t	ZRTPSessLength; /**< length of ZRTP session key depends on agreed hash algorithm */
	uint8_t *exportedKey; /**< computed as in rfc section 4.5.2 only if needed */
	uint8_t exportedKeyLength; /**< length of previous buffer, shall be channel[0]->hashLength */
	uint8_t ZRTPSessContext[24]; /**< computed at the same time as the commit - useful only when a GoClear is sent - described in rfc section 4.7.2.1 -> (ZIDi||ZIDr) */

	/* network */
	size_t mtu; /**< Maximum size in bytes of a ZRTP packet generated locally, has a low limit of BZRTP_MINIMUM_MTU */

};

#ifdef __cplusplus
}
#endif

#endif /* ifndef TYPEDEF_H */
