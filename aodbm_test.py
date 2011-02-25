'''  
    aodbm - Append Only Database Manager
    Copyright (C) 2011 Daniel Waterworth

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

import aodbm, random

#n = 10000

db = aodbm.AODBM("testdb")
ver = db.current_version()
ver['test'] = 'hello'
assert ver['test'] == 'hello'

#ver = db.current_version()
#l = range(n)
#random.shuffle(l)
#for i in l:
#    ver['~' + str(i) + '~'] = 'world' + str(i)
#print ver.version
#print "written"
#ver = aodbm.Version(db, 4575053)
#for i in xrange(n):
#    assert ver['~' + str(i) + '~'] == 'world' + str(i)

