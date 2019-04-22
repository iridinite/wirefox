/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "Enumerations.h"
#include "BinaryStream.h"
#include "WirefoxTime.h"

namespace wirefox {

    class Packet;
    class BinaryStream;

    namespace detail {

        struct PacketHeader;
        struct RemotePeer;
        class Peer;

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a handshake handler.
         * 
         * This is an abstract class that is responsible for dealing with the handshake sequence of a connection.
         * The exact handshake style is left up to a derived class to determine.
         */
        class Handshaker {
        public:
            /// Indicates which party initiated a handshake (or connection).
            enum class Origin {
                INVALID,    ///< Invalid setting. Used by Remote #0 to indicate no handshake.
                SELF,       ///< We initiated the handshake.
                REMOTE      ///< A remote party initiated the handshake.
            };

            /// Represents a handler for sending out handshake fragments. The parameter is a packet payload
            /// that was passed as a movable BinaryStream.
            typedef std::function<void(BinaryStream&& outstream)> ReplyHandler_t;

            /// Represents a callback handler for finishing the handshake (whether with success or failure).
            /// The parameter is a movable Packet that represents a user notification to be posted to the Peer.
            typedef std::function<void(Packet&& notification)> CompletionHandler_t;

            virtual ~Handshaker() = default;

            /// Initiate the handshake.
            virtual void            Begin() = 0;

            /**
             * \brief Handle an incoming packet as a part of an ongoing handshake.
             * 
             * \param[in]   packet  The message that was received from our assigned remote.
             */
            virtual void            Handle(const Packet& packet) = 0;

            /**
             * \brief Finalize the handshake and set its result.
             *
             * \param[in]   result  The end result of this connection attempt.
             */
            void                    Complete(ConnectResult result);

            /// Run timed tasks, particularly resending lost packets and detecting timeouts.
            void                    Update();

            /// Returns a value indicating who initiated this handshake.
            Origin                  GetOrigin() const noexcept { return m_origin; }

            /// Returns a value indicating the current status of the handshake sequence.
            ConnectResult           GetResult() const noexcept { return m_result; }

            /// Returns a boolean indicating whether the handshake has ended (regardless of whether it succeeded or failed).
            bool                    IsDone() const noexcept { return m_result != ConnectResult::IN_PROGRESS; }

            /// Registers a handler that will be responsible for sending messages to the remote endpoint.
            void                    SetReplyHandler(ReplyHandler_t handler);

            /// Registers a handler that will be responsible for posting notifications about success or failure.
            void                    SetCompletionHandler(CompletionHandler_t handler);

        protected:
            /**
             * \brief Constructs a new Handshaker instance.
             * 
             * \param[in]   master  The owner of \p remote. Used for checking for duplicate PeerIDs.
             * \param[in]   remote  The RemotePeer controlled by this Handshaker. Some fields are filled in during handshake.
             * \param[in]   origin  Indicates which endpoint initiated this handshake attempt.
             */
            Handshaker(Peer* master, RemotePeer* remote, Origin origin);

            /**
             * \brief Send a handshake part to the remote endpoint.
             * 
             * \param[in]   outstream   Implementation-defined handshake part payload.
             * \param[in]   isRetry     If false, the number of send retries should be reset.
             */
            void                    Reply(BinaryStream&& outstream, bool isRetry = false);

            RemotePeer*             m_remote;
            Peer*                   m_peer;

        private:
            ReplyHandler_t          m_replyHandler;
            CompletionHandler_t     m_completionHandler;

            Origin                  m_origin;
            ConnectResult           m_result;

            BinaryStream            m_lastReply;
            Timestamp               m_resendNext;
            unsigned int            m_resendAttempts;
        };

        /// \endcond

    }

}
