import aodbm

n = 100

db = aodbm.AODBM("testdb")
ver = db.current_version()
for i in reversed(xrange(n)):
    ver['hello' + str(i)] = 'world' + str(i)
print ver.version
print "written"
for i in xrange(n):
    print ver['hello' + str(i)]

