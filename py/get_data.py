import spiceypy as spice
print(spice.tkvrsn("TOOLKIT"))
spice.furnsh("./data/naif0012.tls.pc")
spice.furnsh("./data/pck00011.tpc")
spice.furnsh("./data/gm_de440.tpc")
print(spice.bodvrd("10", "RADII", 3))

from astropy.table import Table
from astroquery.jplhorizons import Horizons
# BaryCenters
def GetOrbitWithRespectToBody(body_string, location_string):
    
obj = Horizons(id='1', location='SSB', epochs={'start':'2200-01-01', 'stop':'2300-01-01', 'step':'5d'})
e = obj.elements()
e.remove_columns(['targetname', 'datetime_jd', 'datetime_str'])
e.write('1-0.hdf5', path='output', overwrite=True, serialize_meta=False, compressed=True)
