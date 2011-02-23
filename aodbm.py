import ctypes

class Data(ctypes.Structure):
    _fields_ = [("dat", ctypes.c_char_p),
                ("sz", ctypes.c_size_t)]

def str_to_data(st):
    return Data(st, len(st))

def data_to_str(dat):
    return ctypes.string_at(dat.dat, dat.sz)

data_ptr = ctypes.POINTER(Data)

aodbm_lib = ctypes.CDLL("./libaodbm.so")

aodbm_lib.aodbm_open.argtypes = [ctypes.c_char_p]
aodbm_lib.aodbm_open.restype = ctypes.c_void_p

aodbm_lib.aodbm_close.argtypes = [ctypes.c_void_p]
aodbm_lib.aodbm_close.restype = None

aodbm_lib.aodbm_current.argtypes = [ctypes.c_void_p]
aodbm_lib.aodbm_close.restype = ctypes.c_uint64

aodbm_lib.aodbm_commit.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
aodbm_lib.aodbm_commit.restype = ctypes.c_bool

aodbm_lib.aodbm_has.argtypes = [ctypes.c_void_p, ctypes.c_uint64, data_ptr]
aodbm_lib.aodbm_has.restype = ctypes.c_bool

aodbm_lib.aodbm_get.argtypes = [ctypes.c_void_p, ctypes.c_uint64, data_ptr]
aodbm_lib.aodbm_get.restype = data_ptr

aodbm_lib.aodbm_set.argtypes = [ctypes.c_void_p, ctypes.c_uint64, data_ptr, data_ptr]
aodbm_lib.aodbm_set.restype = ctypes.c_uint64

aodbm_lib.aodbm_del.argtypes = [ctypes.c_void_p, ctypes.c_uint64, data_ptr]
aodbm_lib.aodbm_del.restype = ctypes.c_uint64

aodbm_lib.aodbm_data_lt.argtypes = [data_ptr, data_ptr]
aodbm_lib.aodbm_data_lt.restype = ctypes.c_bool

class Version(object):
    def __init__(self, db, version):
        self.db = db
        self.version = version
    
    def has(self, key):
        return aodbm_lib.aodbm_has(self.db.db, self.version, str_to_data(key))
    
    def __getitem__(self, key):
        ptr = aodbm_lib.aodbm_get(self.db.db, self.version, str_to_data(key))
        if ptr:
            return data_to_str(ptr.contents)
        raise KeyError()
    
    def __setitem__(self, key, val):
        key = str_to_data(key)
        val = str_to_data(val)
        self.version = aodbm_lib.aodbm_set(self.db.db, self.version, key, val)
    
    def __delitem__(self, key):
        key = str_to_data(key)
        self.version = aodbm_lib.aodbm_del(self.db.db, self.version, key)
    
    def del_key(self, key):
        key = str_to_data(key)
        return Version(self.db, aodbm_lib.aodbm_del(self.db.db, self.version, key))
    
    def set_record(self, key, val):
        key = str_to_data(key)
        val = str_to_data(val)
        return Version(self.db, aodbm_lib.aodbm_set(self.db.db, self.version, key, val))

class AODBM(object):
    def __init__(self, filename):
        self.db = aodbm_lib.aodbm_open(filename)
    
    def __del__(self):
        aodbm_lib.aodbm_close(self.db)
    
    def current_version(self):
        return Version(self, aodbm_lib.aodbm_current(self.db))
    
    def commit(self, version):
        assert self == version.db
        return aodbm_lib.aodbm_commit(self.db, version.version)

def lt(a, b):
    return aodbm_lib.aodbm_data_lt(str_to_data(a), str_to_data(b))
