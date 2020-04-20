#ifndef IMAP_H
#define IMAP_H
#include "imaputils.hpp"
#include <libetpan/libetpan.h>
#include <string>
#include <functional>

namespace IMAP {

  // forward declaration
  class Session;

  
  class Message {

  public:

    // Message constructor, inputs are the session and the message UID
    Message(Session *session, uint32_t uid);
    /**
     * Get the body of the message. You may chose to either include the headers or not.
     */
    std::string getBody();
    /**
     * Get one of the descriptor fields (subject, from, ...)
     */
    std::string getField(std::string fieldname);
    /**
     * Remove this mail from its mailbox
     */
    void deleteFromMailbox();

  private:
    Session *session; // Pointer to current session
    uint32_t uid; // UID of message
  };
  

  class Session {
  public:
    Session(std::function<void()> updateUI);

    /**
     * Get all messages in the INBOX mailbox terminated by a nullptr (like we did in class)
     */
    Message** getMessages();

    /**
     * connect to the specified server (143 is the standard unencrypte imap port)
     */
    void connect(std::string const& server, size_t port = 143);
  
    /**
     * log in to the server (connect first, then log in)
     */
    void login(std::string const& userid, std::string const& password);

    /**
     * select a mailbox (only one can be selected at any given time)
     * 
     * this can only be performed after login
     */
    void selectMailbox(std::string const& mailbox);

    // Session destructor
    ~Session();


    mailimap *current_mailimap; // Current mailimap

  private:

    friend void IMAP::Message::deleteFromMailbox(); // Friend declaration
    uint32_t get_mail_uid(struct mailimap_msg_att *msg_att) const; // Helper fc to get UID
  
    int nr_of_msgs; // Nr of messages in mailbox   
    Message **msgs; // Pointer to array of pointers pointing to messages in inbox
    std::string mb; // Mailbox of session
    std::function<void()> updateUI; // Function object to refresh UI
    
  };
}

#endif /* IMAP_H */
