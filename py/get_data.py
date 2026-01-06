from astroquery.jplhorizons import Horizons
obj = Horizons(id='1', epochs=2461041.5000000)
elements = obj.elements()
lines = obj._raw_response.split('\n')
dividers = [i for i, line in enumerate(lines) if line.startswith('*')]
allValues = {}
for i in range(dividers[0]+1,dividers[1]):
    line = lines[i]
    if line.startswith('GEOPHYSICAL PROPERTIES')
    count = .count('=')
    if count == 1:
        pair = lines[i].split('=')
        allValues[pair[0]] = pair[1]
        
        
