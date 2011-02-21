import aodbm, random

n = 10000

db = aodbm.AODBM("testdb")
ver = db.current_version()
l = range(n)
random.shuffle(l)
for i in l:
    ver['~' + str(i) + '~'] = 'world' + str(i)
print ver.version
print "written"
#ver = aodbm.Version(db, 4575053)
for i in xrange(n):
    assert ver['~' + str(i) + '~'] == 'world' + str(i)

