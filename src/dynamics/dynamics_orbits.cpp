#include "dynamics_orbits.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <optional>
#include <charconv>

// Source for data and equations, converted to radians for this program https://ssd.jpl.nasa.gov/planets/approx_pos.html

const std::string planetData = R"(
               a              e               I                L            long.peri.      long.node.
           au, au/Cy     rad, rad/Cy     deg, deg/Cy      deg, deg/Cy      deg, deg/Cy     deg, deg/Cy
------------------------------------------------------------------------------------------------------
Mercury   0.38709927      0.20563593      7.00497902      252.25032350     77.45779628     48.33076593
          0.00000037      0.00001906     -0.00594749   149472.67411175      0.16047689     -0.12534081
Venus     0.72333566      0.00677672      3.39467605      181.97909950    131.60246718     76.67984255
          0.00000390     -0.00004107     -0.00078890    58517.81538729      0.00268329     -0.27769418
EM Bary   1.00000261      0.01671123     -0.00001531      100.46457166    102.93768193      0.00000000
          0.00000562     -0.00004392     -0.01294668    35999.37244981      0.32327364      0.00000000
Mars      1.52371034      0.09339410      1.84969142       -4.55343205    -23.94362959     49.55953891
          0.00001847      0.00007882     -0.00813131    19140.30268499      0.44441088     -0.29257343
Jupiter   5.20288700      0.04838624      1.30439695       34.39644051     14.72847983    100.47390909
         -0.00011607     -0.00013253     -0.00183714     3034.74612775      0.21252668      0.20469106
Saturn    9.53667594      0.05386179      2.48599187       49.95424423     92.59887831    113.66242448
         -0.00125060     -0.00050991      0.00193609     1222.49362201     -0.41897216     -0.28867794
Uranus   19.18916464      0.04725744      0.77263783      313.23810451    170.95427630     74.01692503
         -0.00196176     -0.00004397     -0.00242939      428.48202785      0.40805281      0.04240589
Neptune  30.06992276      0.00859048      1.77004347      -55.12002969     44.96476227    131.78422574
          0.00026291      0.00005105      0.00035372      218.45945325     -0.32241464     -0.00508664
------------------------------------------------------------------------------------------------------
EM Bary = Earth/Moon Barycenter
)";

static std::string_view StripSpaces(std::string_view view) {
	size_t first = view.find_first_not_of(' ');
	size_t last = view.find_last_not_of(' ') + 1;
	return view.substr(first, last - first);
}

class TableView {
public:
	TableView(std::string_view data, std::initializer_list<size_t>&& columnWidths) {
		// Get lines
		size_t lineStart{ 0 };
		for (size_t lineEnd = 0; lineEnd < data.size(); lineEnd++) {
			if (data.at(lineEnd) == '\n') {
				int lineLength = lineEnd - lineStart;
				if (lineLength > 0) {
					_rows.push_back(data.substr(lineStart, lineLength));
				}
				lineStart = lineEnd + 1;
			}
		}
		// Get column offsets
		_columnOffsets.reserve(columnWidths.size() + 1);
		_columnOffsets.push_back(0);
		for (size_t width : columnWidths) {
			_columnOffsets.push_back(_columnOffsets.back() + width);
		}
	}
	std::string_view GetCell(size_t column, size_t row) const {
		std::string_view view = GetRow(row);
		if (column > _columnOffsets.size()) {
			throw std::out_of_range("Column index out of range");
		}
		size_t start{ _columnOffsets.at(column) };
		size_t end{ _columnOffsets.at(column + 1) };
		return view.substr(start, end - start);
	}
	float GetCellValue(size_t column, size_t row) const {
		std::string_view view = StripSpaces(GetCell(column, row));
		double value{ NAN };
		std::from_chars(view.data(), view.data() + view.size(), value);
		return value;
	}
	std::string_view GetRow(size_t row) const {
		return _rows.at(row);
	}
	size_t GetRowCount() const {
		return _rows.size();
	}
	size_t GetColumnCount() const {
		return _columnOffsets.size() - 1;
	}
private:
	std::vector<size_t> _columnOffsets;
	std::vector<std::string_view> _rows;
};

static double WrapToRange(double val, double min, double max) {
	assert(max > min);
	double range = max - min;
	val = fmod(val - min, range);
	if (val < 0.0) {
		val += range;
	}
	return val + min;
}

double KEWR::AtTime(double time) const {
	return value + rate * ((time - 2451545.0f) / 36525);
}

const double METRES_PER_AU = 149597870700;

glm::dvec3 KElements::AtTime(double time) const {
	const double a = a_wr.AtTime(time);// Semi-major Axis
	const double e = e_wr.AtTime(time);// Eccentricity
	const double I = glm::radians(I_wr.AtTime(time));// Inclination
	const double L = glm::radians(L_wr.AtTime(time));// Mean Longnitude
	const double wl = glm::radians(lp_wr.AtTime(time));// Longnitude of Perihelion
	const double O = glm::radians(ln_wr.AtTime(time));// Longnitude of Ascending Node
	const double w = wl - O; // Argument of Perihelion
	const double M = WrapToRange(L - wl, -glm::pi<double>(), glm::pi<double>()); // Mean Anomaly
	const double E = GetEccentricAnomaly(e, M);
	// Planets position in its own orbital plane
	glm::dvec3 pos{ 
		a * (cos(E) - e),
		a * sqrt(1.0 - e * e) * sin(E),
		0.0 
	};
	// get coordinates in J2000 ecliptic plane
	//pos = glm::rotateZ(pos, -w);
	//pos = glm::rotateY(pos, -I);
	//pos = glm::rotateZ(pos, -O);
	pos = {
		(cos(w) * cos(O) - sin(w) * sin(O) * cos(I)) * pos.x + (-sin(w) * cos(O) - cos(w) * sin(O) * cos(I)) * pos.y,
		(cos(w) * sin(O) + sin(w) * cos(O) * cos(I)) * pos.x + (-sin(w) * sin(O) + cos(w) * cos(O) * cos(I)) * pos.y,
		(sin(w) * sin(I)) * pos.x + (cos(w) * sin(I)) * pos.y,
	};
	return pos;
}

double KElements::GetEccentricAnomaly(double eccentricity, double meanAnomaly) const {
	double E = meanAnomaly + eccentricity * sin(meanAnomaly);
	int i = 0;
	for (; i < 1000; i++) {// Cap iterations
		const double dM =  E - eccentricity * sin(E) - meanAnomaly;
		const double dE = dM / (1.0 - eccentricity * cos(E));
		E -= dE;
		if (abs(dE) <= 1e-8) {// Error is small enough to stop
			break;
		}
	}
	return E;
}

Planet::Planet(std::string_view name, const KElements& elements) : _name(name), _elements(elements) {}

const std::string& Planet::GetName() const {
	return _name;
}

glm::dvec3 Planet::AtTime(double time) const {
	return _elements.AtTime(time);
}

SolarSystem::SolarSystem() {
	TableView table(planetData, {8, 12, 16, 16, 20, 16, 16});
	auto PlanetFromTable = [&](size_t row) -> Planet* {
		Planet* planet = new Planet(StripSpaces(table.GetCell(0, row)), KElements{
			.a_wr = {table.GetCellValue(1, row), table.GetCellValue(1, row + 1)},
			.e_wr = {table.GetCellValue(2, row), table.GetCellValue(2, row + 1)},
			.I_wr = {table.GetCellValue(3, row), table.GetCellValue(3, row + 1)},
			.L_wr = {table.GetCellValue(4, row), table.GetCellValue(4, row + 1)},
			.lp_wr = {table.GetCellValue(5, row), table.GetCellValue(5, row + 1)},
			.ln_wr = {table.GetCellValue(6, row), table.GetCellValue(6, row + 1)},
		});
		_planets.emplace_back(planet);
		return planet;
	};
	mercury = PlanetFromTable(3);
	venus = PlanetFromTable(5);
	earth = PlanetFromTable(7);
	mars = PlanetFromTable(9);
	jupiter = PlanetFromTable(11);
	saturn = PlanetFromTable(13);
	uranus = PlanetFromTable(15);
	neptune = PlanetFromTable(17);
}
