import aodbm

db = aodbm.AODBM("testdb")
ver = db.current_version()
for i in xrange(5):
    ver['hello' + str(i)] = 'world' + str(i)
print ver.version
print ver['hello4']
