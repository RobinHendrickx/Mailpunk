#include "imap.hpp"
#include <cstring>

IMAP::Session::Session(std::function<void()> updateUI): updateUI(updateUI), nr_of_msgs(0){
  
  current_mailimap = mailimap_new(0, NULL);

}



void IMAP::Session::connect(std::string const& server, size_t port){
  
  check_error(mailimap_socket_connect(current_mailimap, server.c_str(), port),
	      "Could not connect");
  return;

}



void IMAP::Session::login(std::string const& userid, std::string const& password){
  
  check_error(mailimap_login(current_mailimap, userid.c_str(), password.c_str()),
	      "Login Error");
  return;

}



void IMAP::Session::selectMailbox(std::string const& mailbox){
  
  check_error(mailimap_select(current_mailimap, mailbox.c_str()),
	      "error selecting mailbox");
  mb = mailbox;
  return;

}



IMAP::Session::~Session(){
  
  for (int i=0; i<nr_of_msgs; i++)
    {
      delete msgs[i];
    }

  delete [] msgs;
  
  mailimap_logout(current_mailimap);
  mailimap_free(current_mailimap);
  
}

IMAP::Message** IMAP::Session::getMessages(){

  // GET NR OF MESSAGES
  // Create list for status request type
  mailimap_status_att_list *sa_list = mailimap_status_att_list_new_empty();
  check_error(mailimap_status_att_list_add(sa_list, MAILIMAP_STATUS_ATT_MESSAGES),
	      "Error adding status_att to att_list in getMessages");
  
  // Check status of mailbox
  mailimap_mailbox_data_status* status;
  check_error(mailimap_status(current_mailimap, mb.c_str(), sa_list, &status), 
	      "Error getting mailbox nr of messages");
  nr_of_msgs = ((struct mailimap_status_info*)
		clist_content(clist_begin(status->st_info_list)))->st_value;

  // Free memory
  mailimap_mailbox_data_status_free(status);
  mailimap_status_att_list_free(sa_list);

  // GET MESSAGES
  // Create an array of message pointers with a terminating nullptr
  msgs = new Message*[nr_of_msgs+1];
  
  // If zero, return straight away
  if (nr_of_msgs == 0)
    {
      msgs[0] = nullptr;
      return msgs;
    }
    
  // Create a new fetch attribute and add it to fetch structure
  struct mailimap_fetch_type* fch_type = mailimap_fetch_type_new_fetch_att_list_empty();
  struct mailimap_fetch_att* fch_att = mailimap_fetch_att_new_uid();
  check_error(mailimap_fetch_type_new_fetch_att_list_add(fch_type,fch_att),
	      "Error adding fetch att to fetch att list in getMessages");

  // Declare clist, create the set and perform fetch 
  clist* fch_result;
  struct mailimap_set* set = mailimap_set_new_interval(1,0);
  check_error(mailimap_fetch(current_mailimap, set, fch_type, &fch_result),
	      "Error fetching the UIDs"); 	      

  // Loop through the result, get msgs uid and create message
  clistiter* cur;
  int i = 0;
  
  for(cur = clist_begin(fch_result) ; cur != NULL ; cur = clist_next(cur)) {

    struct mailimap_msg_att *msg_att;
    uint32_t uid;
		
    msg_att = (struct mailimap_msg_att*) clist_content(cur);
    uid = get_mail_uid(msg_att);
    msgs[i] = new Message(this, uid);
    i++;
  }
  
  msgs[i] = nullptr; // terminating nullpointer

  // Free memory
  mailimap_fetch_list_free(fch_result);
  mailimap_fetch_type_free(fch_type);
  mailimap_set_free(set);

  return msgs;

}



uint32_t IMAP::Session::get_mail_uid(struct mailimap_msg_att *msg_att) const {

  // Loop through message attribute, check if item static and get uid
  clistiter* cur;
  for (cur = clist_begin(msg_att->att_list); cur != NULL; cur = clist_next(cur))
    {

      struct mailimap_msg_att_item* item = (struct mailimap_msg_att_item*) clist_content(cur);
      if(item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC)
	continue;
      if(item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_UID)
	continue;
      return item->att_data.att_static->att_data.att_uid;
    }
  return 0;
}


IMAP::Message::Message(Session* session, uint32_t uid): session(session), uid(uid){

};



void IMAP::Message::deleteFromMailbox(){
  
  // Add delete flag to flaglist
  mailimap_flag_list *flag_list = mailimap_flag_list_new_empty();
  mailimap_flag *flag = mailimap_flag_new_deleted();
  check_error(mailimap_flag_list_add(flag_list, flag),"Couldn't add delete flag to flaglist");

  // Add delete flag to messages
  mailimap_store_att_flags *store_att_flags = mailimap_store_att_flags_new_set_flags(flag_list);
  mailimap_set *set = mailimap_set_new_single(uid);
  check_error(mailimap_uid_store(session->current_mailimap, set, store_att_flags),
	      "Couldn't add delete flag to msgs");

  // Expunge mailbox
  check_error(mailimap_expunge(session->current_mailimap), "Couldn't expunge mailbox");

  // Delete the array of pointers and the messages it points to
  // Except the current one
  for (int i=0; i<session->nr_of_msgs; i++)
    {
      if(uid == session->msgs[i]->uid)
	continue;
      delete session->msgs[i];
    }

  delete [] session->msgs;

  // Free memory
  mailimap_store_att_flags_free(store_att_flags);
  mailimap_set_free(set);

  // Refresh the UI and delete current Message object
  session->updateUI();
  delete this;
  return;

}


std::string IMAP::Message::getField(std::string fieldname){
  
  // Create fetch type, result and set
  clist* fch_result;
  struct mailimap_set* set = mailimap_set_new_single(uid);
  struct mailimap_fetch_type* fch_type = mailimap_fetch_type_new_fetch_att_list_empty();

  // Add header to fetch
  auto hdr_section = mailimap_section_new_header();
  struct mailimap_fetch_att *hdr_fch_att = mailimap_fetch_att_new_body_peek_section(hdr_section);
  check_error(mailimap_fetch_type_new_fetch_att_list_add(fch_type, hdr_fch_att),
	      "Couldn't add header fch att in getField()");
  
  // Fetch
  check_error(mailimap_uid_fetch(session->current_mailimap, set, fch_type, &fch_result),
	      "Couldnt fetch body and headers in getField()");

  // Loop through result
  struct mailimap_msg_att *msg_att = (struct mailimap_msg_att*) clist_content(clist_begin(fch_result));
  std::string content;
  
  for(clistiter *cur = clist_begin(msg_att->att_list) ; cur != NULL ; cur = clist_next(cur))
    {
      struct mailimap_msg_att_item* item = (mailimap_msg_att_item*) clist_content(cur);
      if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC)
	{
	  continue;
	}
		
      if (item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_BODY_SECTION)
	{
	  continue;
	}		
       
      content = item->att_data.att_static->att_data.att_body_section->sec_body_part;
      
      // Check if empty
      if(content.empty())
      	{
      	  continue;
      	}      
    }
  
  // Free memory
  mailimap_fetch_type_free(fch_type);
  mailimap_fetch_list_free(fch_result);   
  mailimap_set_free(set);  

  // Extract required content of the field
  size_t fname_end, result_end;
    
  if (content.find(fieldname) != std::string::npos)
    {
      fname_end = content.find(fieldname) + fieldname.length(); 

      // strip whitespace behind solution
      while (content[fname_end] == ' ' || content[fname_end] == ':')
	fname_end++;

      // Check if end is newline character  
      if(content[fname_end] =='\n')
	return "<No info>";

      // Substract revelevant part
      result_end = content.find('\n', fname_end);
      content = content.substr(fname_end,result_end-fname_end-1);

      if(fieldname == "Subject")
      	return content;

      else if(fieldname == "From")
	return content;
    }
  
  return "<No info>";

}




std::string IMAP::Message::getBody(){

  // Create fetch type, result and set
  clist* fch_result;
  struct mailimap_set* set = mailimap_set_new_single(uid);
  struct mailimap_fetch_type* fch_type = mailimap_fetch_type_new_fetch_att_list_empty();

  // Add body to fetch
  mailimap_section* body_section = mailimap_section_new(NULL);
  struct mailimap_fetch_att* body_fch_att = mailimap_fetch_att_new_body_peek_section(body_section);  
  check_error(mailimap_fetch_type_new_fetch_att_list_add(fch_type, body_fch_att),
	      "Couldn't add body fch att in getBody()");
 
  // Fetch
  check_error(mailimap_uid_fetch(session->current_mailimap, set, fch_type, &fch_result),
	      "Couldn't fetch body in getBody()");   
  struct mailimap_msg_att *msg_att = (struct mailimap_msg_att*) clist_content(clist_begin(fch_result));

  // Loop through results, check if att_type static and pick correct att_type  
  std::string content;

  for(clistiter *cur = clist_begin(msg_att->att_list) ; cur != NULL ; cur = clist_next(cur))
    {
      struct mailimap_msg_att_item* item = (mailimap_msg_att_item*) clist_content(cur);

      if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC)
	{
	  continue;
	}
		
      if (item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_BODY_SECTION)
	{
	  continue;
	}		
       
      content = item->att_data.att_static->att_data.att_body_section->sec_body_part;

      // Check if empty
      if(content.empty())
	{
	  continue;
	}
    }

  // Free memory
  mailimap_fetch_type_free(fch_type);
  mailimap_fetch_list_free(fch_result);
  mailimap_set_free(set);
  
  return content;

}
