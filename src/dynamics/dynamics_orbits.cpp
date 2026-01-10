#include "dynamics_orbits.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <optional>
#include <charconv>
#include <filesystem>
#include <fstream>

static std::string_view StripSpaces(std::string_view view) {
	size_t first = view.find_first_not_of(' ');
	size_t last = view.find_last_not_of(' ') + 1;
	return view.substr(first, last - first);
}

class TableView {
public:
	TableView(std::string_view tablePath) {
		// Get File
		std::ifstream ifs(std::filesystem::current_path() / "assets" / "data" / tablePath);
		std::string data((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
		// Get lines
		size_t lineStart{ 0 };
		for (size_t lineEnd = 0; lineEnd <= data.size(); lineEnd++) {
			if (lineEnd == data.size() || data.at(lineEnd) == '\n') {
				int lineLength = lineEnd - lineStart;
				auto line = std::string_view(data).substr(lineStart, lineLength);
				// Get cells
				std::vector<std::string> lineCells;
				size_t cellStart{ 0 };
				for (size_t cellEnd = 0; cellEnd < line.size(); cellEnd++) {
					if (line.at(cellEnd) == ',') {
						int cellLength = cellEnd - cellStart;
						lineCells.push_back(std::string(line.substr(cellStart, cellLength)));
						cellStart = cellEnd + 1;
					}
				}
				if (!lineCells.empty()) {
					_cells.push_back(lineCells);
				}
				lineStart = lineEnd + 1;
			}
		}
	}
	std::string_view GetCell(size_t column, size_t row) const {
		return GetRow(row).at(column);
	}
	float GetCellValue(size_t column, size_t row) const {
		std::string_view view = StripSpaces(GetCell(column, row));
		double value{ NAN };
		std::from_chars(view.data(), view.data() + view.size(), value);
		return value;
	}
	const std::vector<std::string>& GetRow(size_t row) const {
		return _cells.at(row);
	}
	size_t GetRowCount() const {
		return _cells.size();
	}
private:
	std::vector<std::vector<std::string>> _cells;
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

double VaryingElement::GetValueAtTime(double time) const {
	return value + rate * ((time - 2451545.0f) / 36525);
}

const double METRES_PER_AU = 149597870700;

static double GetEccentricAnomaly(double eccentricity, double meanAnomaly) {
	double E = meanAnomaly + eccentricity * sin(meanAnomaly);
	int i = 0;
	for (; i < 1000; i++) {// Cap iterations
		const double dM = E - eccentricity * sin(E) - meanAnomaly;
		const double dE = dM / (1.0 - eccentricity * cos(E));
		E -= dE;
		if (abs(dE) <= 1e-8) {// Error is small enough to stop
			break;
		}
	}
	return E;
}

KeplerOrbit::KeplerOrbit(SolarBody* parentBody, double a, double e, double w, double M, double I, double ln) : parentBody(parentBody), a(a), e(e), w(w), M(M), I(I), ln(ln) {
}

glm::dvec3 KeplerOrbit::GetPositionAtTime(double time) const {
	const double E = GetEccentricAnomaly(e, M);
	// Planets position in its own orbital plane
	glm::dvec3 pos{ 
		a * (cos(E) - e),
		a * sqrt(1.0 - e * e) * sin(E),
		0.0 
	};
	// get coordinates in J2000 ecliptic plane
	pos = {
		(cos(w) * cos(ln) - sin(w) * sin(ln) * cos(I)) * pos.x + (-sin(w) * cos(ln) - cos(w) * sin(ln) * cos(I)) * pos.y,
		(cos(w) * sin(ln) + sin(w) * cos(ln) * cos(I)) * pos.x + (-sin(w) * sin(ln) + cos(w) * cos(ln) * cos(I)) * pos.y,
		(sin(w) * sin(I)) * pos.x + (cos(w) * sin(I)) * pos.y,
	};
	if (parentBody) {
		pos += parentBody->GetPositionAtTime(time);
	}
	return pos;
}

VaryingKeplerOrbit::VaryingKeplerOrbit(SolarBody* parentBody, VaryingElement a_wr, VaryingElement e_wr, VaryingElement I_wr, VaryingElement L_wr, VaryingElement lp_wr, VaryingElement ln_wr) : parentBody(parentBody), a_wr(a_wr), e_wr(e_wr), I_wr(I_wr), L_wr(L_wr), lp_wr(lp_wr), ln_wr(ln_wr) {
}

glm::dvec3 VaryingKeplerOrbit::GetPositionAtTime(double time) const {
	const double lp = glm::radians(lp_wr.GetValueAtTime(time));
	const double L = glm::radians(L_wr.GetValueAtTime(time));
	const double ln = glm::radians(ln_wr.GetValueAtTime(time));
	const double I = glm::radians(I_wr.GetValueAtTime(time));
	const double w = lp - ln; // Argument of Perihelion
	const double M = WrapToRange(L - lp, -glm::pi<double>(), glm::pi<double>()); // Mean Anomaly
	KeplerOrbit orbit(parentBody, a_wr.GetValueAtTime(time) * METRES_PER_AU, e_wr.GetValueAtTime(time), w, M, I, ln);
	return orbit.GetPositionAtTime(time);
}

SolarBody::SolarBody(std::string_view name, double radius, SolarBodyDriver* driver) : _name(name), _radius(radius), _driver(driver) {}

const std::string& SolarBody::GetName() const {
	return _name;
}

double SolarBody::GetRadius() const {
	return _radius;
}

glm::dvec3 SolarBody::GetPositionAtTime(double time) const {
	if (_driver) {
		return _driver->GetPositionAtTime(time);
	}
	return glm::dvec3(0.0);
}

SolarSystem::SolarSystem() {
	// Radius: https://ssd.jpl.nasa.gov/bodies/phys_par.html
	TableView planetOrbits("PlanetOrbits.csv");
	sun = AddBody(new SolarBody("Sun", 695'508'000));
	auto PlanetFromTable = [&](size_t row, double radius) -> SolarBody* {
		SolarBody* planet = new SolarBody(StripSpaces(planetOrbits.GetCell(0, row)), radius, new VaryingKeplerOrbit(
			sun,
			{planetOrbits.GetCellValue(1, row), planetOrbits.GetCellValue(1, row + 1)},
			{planetOrbits.GetCellValue(2, row), planetOrbits.GetCellValue(2, row + 1)},
			{planetOrbits.GetCellValue(3, row), planetOrbits.GetCellValue(3, row + 1)},
			{planetOrbits.GetCellValue(4, row), planetOrbits.GetCellValue(4, row + 1)},
			{planetOrbits.GetCellValue(5, row), planetOrbits.GetCellValue(5, row + 1)},
			{planetOrbits.GetCellValue(6, row), planetOrbits.GetCellValue(6, row + 1)}
		));
		return planet;
	};
	mercury = AddBody(PlanetFromTable(2, 2'439'400));
	venus = AddBody(PlanetFromTable(4, 6'051'800));
	earth = AddBody(PlanetFromTable(6, 6'371'008));
	mars = AddBody(PlanetFromTable(8, 3'389'500));
	jupiter = AddBody(PlanetFromTable(10, 69'911'000));
	saturn = AddBody(PlanetFromTable(12, 58'232'000));
	uranus = AddBody(PlanetFromTable(14, 25'362'000));
	neptune = AddBody(PlanetFromTable(16, 24'622'000));
	TableView satOrbits("SatelliteOrbits.csv");
	TableView satConstants("SatelliteConstants.csv");
	auto SatelliteFromTable = [&](SolarBody* parent, size_t row, double radius) -> SolarBody* {
		SolarBody* satellite = new SolarBody(StripSpaces(satOrbits.GetCell(1, row)), radius, new KeplerOrbit(
			parent,
			satOrbits.GetCellValue(5, row) * 1000.0,
			satOrbits.GetCellValue(6, row),
			glm::radians(satOrbits.GetCellValue(7, row)),
			glm::radians(satOrbits.GetCellValue(8, row)),
			glm::radians(satOrbits.GetCellValue(9, row)),
			glm::radians(satOrbits.GetCellValue(10, row))
		));
		return satellite;
	};
	for (int row = 2; row < satOrbits.GetRowCount(); row++) {
		// Check it is ecliptic
		//if (StripSpaces(satOrbits.GetCell(3, row)) != "ecliptic") {
		//	continue;
		//}
		// Find parent
		auto parentName = satOrbits.GetCell(0, row);
		SolarBody* parent = GetBody(parentName);
		if (!parent) {
			continue;
		}
		// Find radius
		auto satName = satOrbits.GetCell(1, row);
		double radius = 0.0f;
		for (int crow = 2; crow < satConstants.GetRowCount(); crow++) {
			if (satConstants.GetCell(1, crow) == satName) {
				radius = satConstants.GetCellValue(4, crow) * 1000.0;
			}
		}
		if (radius == 0.0f) {
			continue;
		}
		AddBody(SatelliteFromTable(parent, row, radius));
	}
}

SolarBody* SolarSystem::GetBody(std::string_view bodyName) {
	for (auto& body : bodies) {
		if (body->GetName() == bodyName) {
			return body.get();
		}
	}
	return nullptr;
}

SolarBody* SolarSystem::AddBody(SolarBody* newSolarBody) {
	std::cout << "Added body [" << newSolarBody->GetName() << "]" << std::endl;
	return bodies.emplace_back(newSolarBody).get();
}
