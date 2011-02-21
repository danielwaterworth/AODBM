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
for i in xrange(n):
    print ver['~' + str(i) + '~']

