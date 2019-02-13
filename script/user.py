class user:
    '玩家数据类'

    def __init__(self, uid):
        self.uid = uid
    temp_data = {}
    save_data = {}
    def get_save_data(self):
        return self.save_data
    def get_temp_data(self):
        return self.temp_data

def get_user_save(uid, save_key):
    if not all_user_list.haskey(uid):
        return None
    userobj = all_user_list[uid]
    if not uid:
        return None
     
    
def get_user_save(uid, save_key):
    
