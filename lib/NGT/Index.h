//
// Copyright (C) 2015-2020 Yahoo Japan Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include	<string>
#include	<vector>
#include	<map>
#include	<set>
#include	<bitset>
#include	<iomanip>
#include	<unordered_set>

#include	<sys/time.h>
#include	<sys/stat.h>
#include	<stdint.h>

#include	"NGT/defines.h"
#include	"NGT/Common.h"
#include	"NGT/Tree.h"
#include	"NGT/Thread.h"
#include	"NGT/Graph.h"


namespace NGT {

  class Property;

  class Index {
  public:

    class Property {
    public:
      typedef ObjectSpace::ObjectType		ObjectType;
      typedef ObjectSpace::DistanceType		DistanceType;
      typedef NeighborhoodGraph::SeedType	SeedType;
      typedef NeighborhoodGraph::GraphType	GraphType;
      enum ObjectAlignment {
	ObjectAlignmentNone	= 0,
	ObjectAlignmentTrue	= 1,
	ObjectAlignmentFalse	= 2
      };
      enum IndexType {
	IndexTypeNone		= 0,
	GraphAndTree		= 1,
	Graph			= 2
      };
      enum DatabaseType {
	DatabaseTypeNone	= 0,
	Memory			= 1,
	MemoryMappedFile	= 2
      };
      Property() { setDefault(); }
      void setDefault() {
	dimension 	= 0;
	threadPoolSize	= 32;
	objectType	= ObjectSpace::ObjectType::Float;
	distanceType	= DistanceType::DistanceTypeL2;
	indexType	= IndexType::GraphAndTree;
	objectAlignment	= ObjectAlignment::ObjectAlignmentFalse;
	pathAdjustmentInterval = 0;
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
	databaseType	= DatabaseType::MemoryMappedFile;
      	graphSharedMemorySize	= 512; // MB
      	treeSharedMemorySize	= 512; // MB
      	objectSharedMemorySize	= 512; // MB  512 is up to 20M objects.
#else
	databaseType	= DatabaseType::Memory;
#endif
	prefetchOffset	= 0;
	prefetchSize	= 0;
      }
      void clear() {
	dimension 	= -1;
	threadPoolSize	= -1;
	objectType	= ObjectSpace::ObjectTypeNone;
	distanceType	= DistanceType::DistanceTypeNone;
	indexType	= IndexTypeNone;
	databaseType	= DatabaseTypeNone;
	objectAlignment	= ObjectAlignment::ObjectAlignmentNone;
	pathAdjustmentInterval	= -1;
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
      	graphSharedMemorySize	= -1;
      	treeSharedMemorySize	= -1;
      	objectSharedMemorySize	= -1;
#endif
	prefetchOffset	= -1;
	prefetchSize	= -1;
	accuracyTable	= "";
      }

      void exportProperty(NGT::PropertySet &p) {
	p.set("Dimension", dimension);
	p.set("ThreadPoolSize", threadPoolSize);
	switch (objectType) {
	case ObjectSpace::ObjectType::Uint8: p.set("ObjectType", "Integer-1"); break;
	case ObjectSpace::ObjectType::Float: p.set("ObjectType", "Float-4"); break;
	default : std::cerr << "Fatal error. Invalid object type. " << objectType << std::endl; abort();
	}
	switch (distanceType) {
	case DistanceType::DistanceTypeNone:			p.set("DistanceType", "None"); break;
	case DistanceType::DistanceTypeL1:			p.set("DistanceType", "L1"); break;
	case DistanceType::DistanceTypeL2:			p.set("DistanceType", "L2"); break;
	case DistanceType::DistanceTypeHamming:			p.set("DistanceType", "Hamming"); break;
	case DistanceType::DistanceTypeJaccard:			p.set("DistanceType", "Jaccard"); break;
	case DistanceType::DistanceTypeAngle:			p.set("DistanceType", "Angle"); break;
	case DistanceType::DistanceTypeCosine:			p.set("DistanceType", "Cosine"); break;
	case DistanceType::DistanceTypeNormalizedAngle:		p.set("DistanceType", "NormalizedAngle"); break;
	case DistanceType::DistanceTypeNormalizedCosine:	p.set("DistanceType", "NormalizedCosine"); break;
	default : std::cerr << "Fatal error. Invalid distance type. " << distanceType << std::endl; abort();
	}
	switch (indexType) {      
	case IndexType::GraphAndTree:	p.set("IndexType", "GraphAndTree"); break;
	case IndexType::Graph:		p.set("IndexType", "Graph"); break;
	default : std::cerr << "Fatal error. Invalid index type. " << indexType << std::endl; abort();
	}
	switch (databaseType) {
	case DatabaseType::Memory:		p.set("DatabaseType", "Memory"); break;
	case DatabaseType::MemoryMappedFile:	p.set("DatabaseType", "MemoryMappedFile"); break;
	default : std::cerr << "Fatal error. Invalid database type. " << databaseType << std::endl; abort();
	}
	switch (objectAlignment) {
	case ObjectAlignment::ObjectAlignmentNone:	p.set("ObjectAlignment", "None"); break;
	case ObjectAlignment::ObjectAlignmentTrue:	p.set("ObjectAlignment", "True"); break;
	case ObjectAlignment::ObjectAlignmentFalse:	p.set("ObjectAlignment", "False"); break;
	default : std::cerr << "Fatal error. Invalid objectAlignment. " << objectAlignment << std::endl; abort();
	}
	p.set("PathAdjustmentInterval", pathAdjustmentInterval);
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
	p.set("GraphSharedMemorySize", graphSharedMemorySize);
	p.set("TreeSharedMemorySize", treeSharedMemorySize);
	p.set("ObjectSharedMemorySize", objectSharedMemorySize);
#endif
	p.set("PrefetchOffset", prefetchOffset);
	p.set("PrefetchSize", prefetchSize);
	p.set("AccuracyTable", accuracyTable);
      }

      void importProperty(NGT::PropertySet &p) {
	setDefault();
	dimension = p.getl("Dimension", dimension);
	threadPoolSize = p.getl("ThreadPoolSize", threadPoolSize);
	PropertySet::iterator it = p.find("ObjectType");
	if (it != p.end()) {
	  if (it->second == "Float-4") {
	    objectType = ObjectSpace::ObjectType::Float;
	  } else if (it->second == "Integer-1") {
	    objectType = ObjectSpace::ObjectType::Uint8;
	  } else {
	    std::cerr << "Invalid Object Type in the property. " << it->first << ":" << it->second << std::endl;
	  }
	} else {
	  std::cerr << "Not found \"ObjectType\"" << std::endl;
	}
	it = p.find("DistanceType");
	if (it != p.end()) {
	  if (it->second == "None") {
	    distanceType = DistanceType::DistanceTypeNone;
	  } else if (it->second == "L1") {
	    distanceType = DistanceType::DistanceTypeL1;
	  } else if (it->second == "L2") {
	    distanceType = DistanceType::DistanceTypeL2;
	  } else if (it->second == "Hamming") {
	    distanceType = DistanceType::DistanceTypeHamming;
	  } else if (it->second == "Jaccard") {
	    distanceType = DistanceType::DistanceTypeJaccard;
	  } else if (it->second == "Angle") {
	    distanceType = DistanceType::DistanceTypeAngle;
	  } else if (it->second == "Cosine") {
	    distanceType = DistanceType::DistanceTypeCosine;
	  } else if (it->second == "NormalizedAngle") {
	    distanceType = DistanceType::DistanceTypeNormalizedAngle;
	  } else if (it->second == "NormalizedCosine") {
	    distanceType = DistanceType::DistanceTypeNormalizedCosine;
	  } else {
	    std::cerr << "Invalid Distance Type in the property. " << it->first << ":" << it->second << std::endl;
	  }
	} else {
	  std::cerr << "Not found \"DistanceType\"" << std::endl;
	}
	it = p.find("IndexType");
	if (it != p.end()) {
	  if (it->second == "GraphAndTree") {
	    indexType = IndexType::GraphAndTree;
	  } else if (it->second == "Graph") {
	    indexType = IndexType::Graph;
	  } else {
	    std::cerr << "Invalid Index Type in the property. " << it->first << ":" << it->second << std::endl;
	  }
	} else {
	  std::cerr << "Not found \"IndexType\"" << std::endl;
	}
	it = p.find("DatabaseType");
	if (it != p.end()) {
	  if (it->second == "Memory") {
	    databaseType = DatabaseType::Memory;
	  } else if (it->second == "MemoryMappedFile") {
	    databaseType = DatabaseType::MemoryMappedFile;
	  } else {
	    std::cerr << "Invalid Database Type in the property. " << it->first << ":" << it->second << std::endl;
	  }
	} else {
	  std::cerr << "Not found \"DatabaseType\"" << std::endl;
	}
	it = p.find("ObjectAlignment");
	if (it != p.end()) {
	  if (it->second == "None") {
	    objectAlignment = ObjectAlignment::ObjectAlignmentNone;
	  } else if (it->second == "True") {
	    objectAlignment = ObjectAlignment::ObjectAlignmentTrue;
	  } else if (it->second == "False") {
	    objectAlignment = ObjectAlignment::ObjectAlignmentFalse;
	  } else {
	    std::cerr << "Invalid Object Alignment in the property. " << it->first << ":" << it->second << std::endl;
	  }
	} else {
	  std::cerr << "Not found \"ObjectAlignment\"" << std::endl;
	  objectAlignment = ObjectAlignment::ObjectAlignmentFalse;
	}
	pathAdjustmentInterval  = p.getl("PathAdjustmentInterval", pathAdjustmentInterval);
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
	graphSharedMemorySize  = p.getl("GraphSharedMemorySize", graphSharedMemorySize);
	treeSharedMemorySize   = p.getl("TreeSharedMemorySize", treeSharedMemorySize);
	objectSharedMemorySize = p.getl("ObjectSharedMemorySize", objectSharedMemorySize);
#endif
	prefetchOffset = p.getl("PrefetchOffset", prefetchOffset);
	prefetchSize = p.getl("PrefetchSize", prefetchSize);
	it = p.find("AccuracyTable");
	if (it != p.end()) {
	  accuracyTable = it->second;
	}
	it = p.find("SearchType");
	if (it != p.end()) {
	  searchType = it->second;
	}
      }

      void set(NGT::Property &prop);
      void get(NGT::Property &prop);
      int		dimension;
      int		threadPoolSize;
      ObjectSpace::ObjectType	objectType;
      DistanceType	distanceType;
      IndexType		indexType;
      DatabaseType	databaseType;
      ObjectAlignment	objectAlignment;
      int		pathAdjustmentInterval;
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
      int		graphSharedMemorySize;
      int		treeSharedMemorySize;
      int		objectSharedMemorySize;
#endif
      int		prefetchOffset;
      int		prefetchSize;
      std::string	accuracyTable;
      std::string	searchType;	// test
    };

    class InsertionResult {
    public:
      InsertionResult():id(0), identical(false), distance(0.0) {}
      InsertionResult(size_t i, bool tf, Distance d):id(i), identical(tf), distance(d) {}
      size_t	id;
      bool	identical;
      Distance	distance; // the distance between the centroid and the inserted object.
    };

    class AccuracyTable  {
    public:
      AccuracyTable() {};
      AccuracyTable(std::vector<std::pair<float, double>> &t) { set(t); }
      AccuracyTable(std::string str) { set(str); }
      void set(std::vector<std::pair<float, double>> &t) { table = t; }
      void set(std::string str) {
	std::vector<std::string> tokens;
	Common::tokenize(str, tokens, ",");
	if (tokens.size() < 2) {
	  return;
	}
	for (auto i = tokens.begin(); i != tokens.end(); ++i) {
	  std::vector<std::string> ts;
	  Common::tokenize(*i, ts, ":");
	  if (ts.size() != 2) {
	    std::stringstream msg;
	    msg << "AccuracyTable: Invalid accuracy table string " << *i << ":" << str;
	    NGTThrowException(msg);
	  }
	  table.push_back(std::make_pair(Common::strtod(ts[0]), Common::strtod(ts[1])));
	}
      }

      float getEpsilon(double accuracy) {
	if (table.size() <= 2) {
	  std::stringstream msg;
	  msg << "AccuracyTable: The accuracy table is not set yet. The table size=" << table.size();
	  NGTThrowException(msg);
	}
	if (accuracy > 1.0) {
	  accuracy = 1.0;
	}
	std::pair<float, double> lower, upper;
	{
	  auto i = table.begin();
	  for (; i != table.end(); ++i) {
	    if ((*i).second >= accuracy) {
	      break;
	    }
	  }
	  if (table.end() == i) {
	    i -= 2;
	  } else if (table.begin() != i) {
	    i--;
	  }
	  lower = *i++;
	  upper = *i;
	}
	float e = lower.first +  (upper.first - lower.first) * (accuracy - lower.second) / (upper.second - lower.second);
	if (e < -0.9) {
	  e = -0.9;
	}
	return e;
      }

      std::string getString() {
	std::stringstream str;
	for (auto i = table.begin(); i != table.end(); ++i) {
	  str << (*i).first << ":" << (*i).second;
	  if (i + 1 != table.end()) {
	    str << ",";
	  }
	}
	return str.str();
      }
      std::vector<std::pair<float, double>> table;
    };

    Index():index(0) {}
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
    Index(NGT::Property &prop, const std::string &database);
#else
    Index(NGT::Property &prop);
#endif
    Index(const std::string &database, bool rdOnly = false):index(0) { open(database, rdOnly); }
    Index(const std::string &database, NGT::Property &prop):index(0) { open(database, prop);  }
    virtual ~Index() { close(); }

    void open(const std::string &database, NGT::Property &prop) {
      open(database);
      setProperty(prop);
    }
    void open(const std::string &database, bool rdOnly = false);

    void close() {
      if (index != 0) { 
	delete index;
	index = 0;
      } 
      path.clear();
    }
    void save() {
      if (path.empty()) {
	NGTThrowException("NGT::Index::saveIndex: path is empty");	
      }
      saveIndex(path);
    }
    static void mkdir(const std::string &dir) { 
      if (::mkdir(dir.c_str(), S_IRWXU | S_IRGRP | S_IXGRP |  S_IROTH | S_IXOTH) != 0) {
	std::stringstream msg;
	msg << "NGT::Index::mkdir: Cannot make the specified directory. " << dir;
	NGTThrowException(msg);	
      }
    }
    static void createGraphAndTree(const std::string &database, NGT::Property &prop, const std::string &dataFile, size_t dataSize = 0, bool redirect = false);
    static void createGraphAndTree(const std::string &database, NGT::Property &prop, bool redirect = false) { createGraphAndTree(database, prop, "", redirect); }
    static void createGraph(const std::string &database, NGT::Property &prop, const std::string &dataFile, size_t dataSize = 0, bool redirect = false);
    template<typename T> size_t insert(std::vector<T> &object);
    template<typename T> size_t append(std::vector<T> &object);
    static void append(const std::string &database, const std::string &dataFile, size_t threadSize, size_t dataSize); 
    static void append(const std::string &database, const float *data, size_t dataSize, size_t threadSize);
    static void remove(const std::string &database, std::vector<ObjectID> &objects, bool force = false);
    static void exportIndex(const std::string &database, const std::string &file);
    static void importIndex(const std::string &database, const std::string &file);
    virtual void load(const std::string &ifile, size_t dataSize) { getIndex().load(ifile, dataSize); }
    virtual void append(const std::string &ifile, size_t dataSize) { getIndex().append(ifile, dataSize); }
    virtual void append(const float *data, size_t dataSize) { 
      redirector.begin();
      try {
	getIndex().append(data, dataSize); 
      } catch(Exception &err) {
	redirector.end();
	throw err;
      }
      redirector.end();
    }
    virtual void append(const double *data, size_t dataSize) { 
      redirector.begin();
      try {
	getIndex().append(data, dataSize); 
      } catch(Exception &err) {
	redirector.end();
	throw err;
      }
      redirector.end();
    }
    virtual size_t getObjectRepositorySize() { return getIndex().getObjectRepositorySize(); }
    virtual void createIndex(size_t threadNumber) {
      redirector.begin();
      try {
	getIndex().createIndex(threadNumber); 
      } catch(Exception &err) {
	redirector.end();
	throw err;
      }
      redirector.end();
    }
    virtual void saveIndex(const std::string &ofile) { getIndex().saveIndex(ofile); }
    virtual void loadIndex(const std::string &ofile) { getIndex().loadIndex(ofile); }
    virtual Object *allocateObject(const std::string &textLine, const std::string &sep) { return getIndex().allocateObject(textLine, sep); }
    virtual Object *allocateObject(const std::vector<double> &obj) { return getIndex().allocateObject(obj); }
    virtual Object *allocateObject(const std::vector<float> &obj) { return getIndex().allocateObject(obj); }
    virtual Object *allocateObject(const std::vector<uint8_t> &obj) { return getIndex().allocateObject(obj); }
    virtual Object *allocateObject(const float *obj, size_t size) { return getIndex().allocateObject(obj, size); }
    virtual size_t getSizeOfElement() { return getIndex().getSizeOfElement(); }
    virtual void setProperty(NGT::Property &prop) { getIndex().setProperty(prop); }
    virtual void getProperty(NGT::Property &prop) { getIndex().getProperty(prop); }
    virtual void deleteObject(Object *po) { getIndex().deleteObject(po); }
    virtual void linearSearch(NGT::SearchContainer &sc) { getIndex().linearSearch(sc); }
    virtual void linearSearch(NGT::SearchQuery &sc) { getIndex().linearSearch(sc); }
    virtual void search(NGT::SearchContainer &sc) { getIndex().search(sc); }
    virtual void search(NGT::SearchQuery &sc) { getIndex().search(sc); }
    virtual void search(NGT::SearchContainer &sc, ObjectDistances &seeds) { getIndex().search(sc, seeds); }
    virtual void remove(ObjectID id, bool force = false) { getIndex().remove(id, force); }
    virtual void exportIndex(const std::string &file) { getIndex().exportIndex(file); }
    virtual void importIndex(const std::string &file) { getIndex().importIndex(file); }
    virtual bool verify(std::vector<uint8_t> &status, bool info = false, char mode = '-') { return getIndex().verify(status, info, mode); }
    virtual ObjectSpace &getObjectSpace() { return getIndex().getObjectSpace(); }
    virtual size_t getSharedMemorySize(std::ostream &os, SharedMemoryAllocator::GetMemorySizeType t = SharedMemoryAllocator::GetTotalMemorySize) {
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
      size_t osize = getObjectSpace().getRepository().getAllocator().getMemorySize(t);
#else
      size_t osize = 0;
#endif
      os << "object=" << osize << std::endl;
      size_t isize = getIndex().getSharedMemorySize(os, t); 
      return osize + isize;
    }
    float getEpsilonFromExpectedAccuracy(double accuracy);
    void searchUsingOnlyGraph(NGT::SearchContainer &sc) { 
      sc.distanceComputationCount = 0;
      sc.visitCount = 0;
      ObjectDistances seeds; 
      getIndex().search(sc, seeds); 
    }
    Index &getIndex() {
      if (index == 0) {
	assert(index != 0);
	NGTThrowException("NGT::Index::getIndex: Index is unavailable.");	
      }
      return *index;
    }
    void enableLog() { redirector.disable(); }
    void disableLog() { redirector.enable(); }

    static void destroy(const std::string &path) {
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
      std::remove(std::string(path + "/grp").c_str());
      std::remove(std::string(path + "/grpc").c_str());
      std::remove(std::string(path + "/trei").c_str());
      std::remove(std::string(path + "/treic").c_str());
      std::remove(std::string(path + "/trel").c_str());
      std::remove(std::string(path + "/trelc").c_str());
      std::remove(std::string(path + "/objpo").c_str());
      std::remove(std::string(path + "/objpoc").c_str());
#else
      std::remove(std::string(path + "/grp").c_str());
      std::remove(std::string(path + "/tre").c_str());
      std::remove(std::string(path + "/obj").c_str());
#endif
      std::remove(std::string(path + "/prf").c_str());
      std::remove(path.c_str());
    }
    
    static void version(std::ostream &os);
    static std::string getVersion();
    std::string getPath(){ return path; }

  protected:
    Object *allocateObject(void *vec, const std::type_info &objectType) {
      if (vec == 0) {
	std::stringstream msg;
	msg << "NGT::Index::allocateObject: Object is not set. ";
	NGTThrowException(msg);	
      }
      Object *object = 0;
      if (objectType == typeid(float)) {
	object = allocateObject(*static_cast<std::vector<float>*>(vec));
      } else if (objectType == typeid(double)) {
	object = allocateObject(*static_cast<std::vector<double>*>(vec));
      } else if (objectType == typeid(uint8_t)) {
	object = allocateObject(*static_cast<std::vector<uint8_t>*>(vec));
      } else {
	std::stringstream msg;
	msg << "NGT::Index::allocateObject: Unavailable object type.";
	NGTThrowException(msg);	
      }
      return object;
    }

    static void loadAndCreateIndex(Index &index, const std::string &database, const std::string &dataFile,
				   size_t threadSize, size_t dataSize);

    Index *index;
    std::string path;
    StdOstreamRedirector redirector;
  };

  class GraphIndex : public Index, 
    public NeighborhoodGraph {
  public:

#ifdef NGT_SHARED_MEMORY_ALLOCATOR
    GraphIndex(const std::string &allocator, bool rdOnly = false);
    GraphIndex(const std::string &allocator, NGT::Property &prop):readOnly(false) {
      initialize(allocator, prop);
    }
    void initialize(const std::string &allocator, NGT::Property &prop);
#else // NGT_SHARED_MEMORY_ALLOCATOR
    GraphIndex(const std::string &database, bool rdOnly = false);
    GraphIndex(NGT::Property &prop):readOnly(false) {
      initialize(prop);
    }

    void initialize(NGT::Property &prop) {
      constructObjectSpace(prop);
      setProperty(prop);
    }

#endif // NGT_SHARED_MEMORY_ALLOCATOR

    virtual ~GraphIndex() {
      destructObjectSpace();
    }
    void constructObjectSpace(NGT::Property &prop);

    void destructObjectSpace() {
      if (objectSpace == 0) {
	return;
      }
      if (property.objectType == NGT::ObjectSpace::ObjectType::Float) {
	ObjectSpaceRepository<float, double> *os = (ObjectSpaceRepository<float, double>*)objectSpace;
#ifndef NGT_SHARED_MEMORY_ALLOCATOR
	os->deleteAll();
#endif
	delete os;
      } else if (property.objectType == NGT::ObjectSpace::ObjectType::Uint8) {
	ObjectSpaceRepository<unsigned char, int> *os = (ObjectSpaceRepository<unsigned char, int>*)objectSpace;
#ifndef NGT_SHARED_MEMORY_ALLOCATOR
	os->deleteAll();
#endif
	delete os;
      } else {
	std::cerr << "Cannot find Object Type in the property. " << property.objectType << std::endl;
	return;
      }
      objectSpace = 0;
    }

    virtual void load(const std::string &ifile, size_t dataSize = 0) {
      if (ifile.empty()) {
	return;
      }
      std::istream *is;
      std::ifstream *ifs = 0;
      if (ifile == "-") {
	is = &std::cin;
      } else {
	ifs = new std::ifstream;
	ifs->std::ifstream::open(ifile);
	if (!(*ifs)) {
	  std::stringstream msg;
	  msg << "Index::load: Cannot open the specified file. " << ifile;
	  NGTThrowException(msg);
	}
	is = ifs;
      }
      try {
	objectSpace->readText(*is, dataSize);
      } catch(Exception &err) {
	if (ifile != "-") {
	  delete ifs;
	}
	throw(err);
      }
      if (ifile != "-") {
	delete ifs;
      }
    }

    virtual void append(const std::string &ifile, size_t dataSize = 0) {
      std::ifstream is(ifile.c_str());
      objectSpace->appendText(is, dataSize);
    }

    virtual void append(const float *data, size_t dataSize) { objectSpace->append(data, dataSize); }
    virtual void append(const double *data, size_t dataSize) { objectSpace->append(data, dataSize); }

    virtual void saveIndex(const std::string &ofile) {
#ifndef NGT_SHARED_MEMORY_ALLOCATOR
      try {
	mkdir(ofile);
      } catch(...) {}
      if (objectSpace != 0) {
	objectSpace->serialize(ofile + "/obj");
      } else {
	std::cerr << "saveIndex::Warning! ObjectSpace is null. continue saving..." << std::endl;
      }
      std::string fname = ofile + "/grp";
      std::ofstream osg(fname);
      if (!osg.is_open()) {
	std::stringstream msg;
	msg << "saveIndex:: Cannot open. " << fname;
	NGTThrowException(msg);
      }
      repository.serialize(osg);
#endif
      saveProperty(ofile);
    }

    void saveProperty(const std::string &file) {
      NGT::PropertySet prop;
      assert(property.dimension != 0);
      GraphIndex::property.exportProperty(prop);
      NeighborhoodGraph::property.exportProperty(prop);
      prop.save(file + "/prf");
    }

    void exportProperty(const std::string &file) {
      NGT::PropertySet prop;
      assert(property.dimension != 0);
      GraphIndex::property.exportProperty(prop);
      NeighborhoodGraph::property.exportProperty(prop);
      prop.save(file + "/prf");
    }

    virtual void loadIndex(const std::string &ifile, bool readOnly);

    virtual void exportIndex(const std::string &ofile) {
      try {
	mkdir(ofile);
      } catch(...) {
	std::stringstream msg;
	msg << "exportIndex:: Cannot make the directory. " << ofile;
	NGTThrowException(msg);
      }
      objectSpace->serializeAsText(ofile + "/obj");
      std::ofstream osg(ofile + "/grp");
      repository.serializeAsText(osg);
      exportProperty(ofile);
    }

    virtual void importIndex(const std::string &ifile) {
      objectSpace->deserializeAsText(ifile + "/obj");
      std::string fname = ifile + "/grp";
      std::ifstream isg(fname);
      if (!isg.is_open()) {
	std::stringstream msg;
	msg << "importIndex:: Cannot open. " << fname;
	NGTThrowException(msg);
      }
      repository.deserializeAsText(isg);
    }

    void linearSearch(NGT::SearchContainer &sc) {
      ObjectSpace::ResultSet results;
      objectSpace->linearSearch(sc.object, sc.radius, sc.size, results);
      ObjectDistances &qresults = sc.getResult();
      qresults.moveFrom(results);
    }

    void linearSearch(NGT::SearchQuery &searchQuery) {
      Object *query = Index::allocateObject(searchQuery.getQuery(), searchQuery.getQueryType());
      try {
        NGT::SearchContainer sc(searchQuery, *query);
	ObjectSpace::ResultSet results;
	objectSpace->linearSearch(sc.object, sc.radius, sc.size, results);
	ObjectDistances &qresults = sc.getResult();
	qresults.moveFrom(results);
      } catch(Exception &err) {
	deleteObject(query);
	throw err;
      }
      deleteObject(query);
    }

    // GraphIndex
    virtual void search(NGT::SearchContainer &sc) {
      sc.distanceComputationCount = 0;
      sc.visitCount = 0;
      ObjectDistances seeds;
      search(sc, seeds);
    }

    void search(NGT::SearchQuery &searchQuery) {
      Object *query = Index::allocateObject(searchQuery.getQuery(), searchQuery.getQueryType());
      try {
        NGT::SearchContainer sc(searchQuery, *query);
	sc.distanceComputationCount = 0;
	sc.visitCount = 0;
	ObjectDistances seeds;
	search(sc, seeds);
      } catch(Exception &err) {
	deleteObject(query);
	throw err;
      }
      deleteObject(query);
    }

    // get randomly nodes as seeds.
    template<class REPOSITORY> void getRandomSeeds(REPOSITORY &repo, ObjectDistances &seeds, size_t seedSize) {
      // clear all distances to find the same object as a randomized object.
      for (ObjectDistances::iterator i = seeds.begin(); i != seeds.end(); i++) {
	(*i).distance = 0.0;
      }
      size_t repositorySize = repo.size();
      repositorySize = repositorySize == 0 ? 0 : repositorySize - 1; // Because the head of repository is a dummy.
      seedSize = seedSize > repositorySize ? repositorySize : seedSize;
      std::vector<ObjectID> deteted;
      while (seedSize > seeds.size()) {
	double random = ((double)rand() + 1.0) / ((double)RAND_MAX + 2.0);
	size_t idx = floor(repositorySize * random) + 1;
	if (repo.isEmpty(idx)) {
	  continue;
	}
	ObjectDistance obj(idx, 0.0);
	if (find(seeds.begin(), seeds.end(), obj) != seeds.end()) {
	  continue;
	}
	seeds.push_back(obj);
      }
    }

    void remove(const ObjectID id, bool force) {
      removeEdgesReliably(id);
      try {
	getObjectRepository().remove(id);
      } catch(Exception &err) {
	std::cerr << "NGT::GraphIndex::remove:: cannot remove from feature. id=" << id << " " << err.what() << std::endl;
	throw err;
      }
    }

    virtual void searchForNNGInsertion(Object &po, ObjectDistances &result) {
      NGT::SearchContainer sc(po);
      sc.setResults(&result);
      sc.size = NeighborhoodGraph::property.edgeSizeForCreation;
      sc.radius = FLT_MAX;
      sc.explorationCoefficient = NeighborhoodGraph::property.insertionRadiusCoefficient;
      try {
	GraphIndex::search(sc);
      } catch(Exception &err) {
	throw err;
      }
      if (static_cast<int>(result.size()) < NeighborhoodGraph::property.edgeSizeForCreation && 
	  result.size() < repository.size()) {
	if (sc.edgeSize != 0) {
	  sc.edgeSize = 0;	// not prune edges.
	  try {
	    GraphIndex::search(sc);
	  } catch(Exception &err) {
	    throw err;
	  }
	}
      }
    }

    void searchForKNNGInsertion(Object &po, ObjectID id, ObjectDistances &result) {
      double radius = FLT_MAX;
      size_t size = NeighborhoodGraph::property.edgeSizeForCreation;
      if (id > 0) {
	size = NeighborhoodGraph::property.edgeSizeForCreation + 1;
      }
      ObjectSpace::ResultSet rs;
      objectSpace->linearSearch(po, radius, size, rs);
      result.moveFrom(rs, id);
      if ((size_t)NeighborhoodGraph::property.edgeSizeForCreation != result.size()) {
	std::cerr << "searchForKNNGInsert::Warning! inconsistency of the sizes. ID=" << id 
	     << " " << NeighborhoodGraph::property.edgeSizeForCreation << ":" << result.size() << std::endl;
	for (size_t i = 0; i < result.size(); i++) {
	  std::cerr << result[i].id << ":" << result[i].distance << " ";
	}
	std::cerr << std::endl;
      }
    }

    virtual void insert(
			ObjectID id
			) {
      ObjectRepository &fr = objectSpace->getRepository();
      if (fr[id] == 0) {
	std::cerr << "NGTIndex::insert empty " << id << std::endl;
	return;
      }
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
      Object &po = *objectSpace->allocateObject(*fr[id]);
#else
      Object &po = *fr[id];
#endif
      ObjectDistances rs;
      if (NeighborhoodGraph::property.graphType == NeighborhoodGraph::GraphTypeANNG) {
	searchForNNGInsertion(po, rs);
      } else {
	searchForKNNGInsertion(po, id, rs);
      }
      insertNode(id, rs);
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
      objectSpace->deleteObject(&po);
#endif
    }

    virtual void createIndex();
    virtual void createIndex(size_t threadNumber);

    void checkGraph()
    {
      GraphRepository &repo = repository;
      ObjectRepository &fr = objectSpace->getRepository();
      for (size_t id = 0; id < fr.size(); id++){
	if (repo[id] == 0) {
	  std::cerr << id << " empty" << std::endl;
	  continue;
	}
	if ((id % 10000) == 0) {
	  std::cerr << "checkGraph: Processed size=" << id << std::endl;
	}
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
	Object &po = *objectSpace->allocateObject(*fr[id]);
#else
	Object &po = *fr[id];
#endif
	GraphNode *objects = getNode(id);

	ObjectDistances rs;
	NeighborhoodGraph::property.edgeSizeForCreation = objects->size() + 1;
	searchForNNGInsertion(po, rs);
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
	objectSpace->deleteObject(&po);
#endif

	if (rs.size() != objects->size()) {
	  std::cerr << "Cannot get the specified number of the results. " << rs.size() << ":" << objects->size() << std::endl;
	}
	size_t count = 0;
	ObjectDistances::iterator rsi = rs.begin(); 
#if defined(NGT_SHARED_MEMORY_ALLOCATOR)
	for (GraphNode::iterator ri = objects->begin(repo.allocator);
	     ri != objects->end(repo.allocator) && rsi != rs.end();) {
#else
	for (GraphNode::iterator ri = objects->begin();
	     ri != objects->end() && rsi != rs.end();) {
#endif
	  if ((*ri).distance == (*rsi).distance && (*ri).id == (*rsi).id) {
	    count++;
	    ri++;
	    rsi++;
	  } else if ((*ri).distance < (*rsi).distance) {
	    ri++;
	  } else {
	    rsi++;
	  }
	}
	if (count != objects->size()) {
	  std::cerr << "id=" << id << " identities=" << count << " " << objects->size() << " " << rs.size() << std::endl;
	}
      }
    }

    virtual bool verify(std::vector<uint8_t> &status, bool info)
    {
      bool valid = true;
      std::cerr << "Started verifying graph and objects" << std::endl;
      GraphRepository &repo = repository;
      ObjectRepository &fr = objectSpace->getRepository();
      if (repo.size() != fr.size()) {
	if (info) {
	  std::cerr << "Warning! # of nodes is different from # of objects. " << repo.size() << ":" << fr.size() << std::endl;
	}
      }
      status.clear();
      status.resize(fr.size(), 0);
      for (size_t id = 1; id < fr.size(); id++){
	status[id] |= repo[id] != 0 ? 0x02 : 0x00;
	status[id] |= fr[id]   != 0 ? 0x01 : 0x00;
      }
      for (size_t id = 1; id < fr.size(); id++) {
	if (fr[id] == 0) {
	  if (id < repo.size() && repo[id] != 0) {
	    std::cerr << "Error! The node exists in the graph, but the object does not exist. " << id << std::endl;
	    valid = false;
	  }
	}
	if (fr[id] != 0 && repo[id] == 0) {
	  std::cerr << "Error. No." << id << " is not registerd in the graph." << std::endl;
	  valid = false;
	}
	if ((id % 1000000) == 0) {
	  std::cerr << "  verified " << id << " entries." << std::endl;
	}
	if (fr[id] != 0) {
	  try {
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
	    Object *po = objectSpace->allocateObject(*fr[id]);
#else
	    Object *po = fr[id];
#endif
	    if (po == 0) {
	      std::cerr << "Error! Cannot get the object. " << id << std::endl;
	      valid = false;
	      continue;
	    }
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
	    objectSpace->deleteObject(po);
#endif
	  } catch (Exception &err) {
	    std::cerr << "Error! Cannot get the object. " << id << " " << err.what() << std::endl;
	    valid = false;
	    continue;
	  }
	}
	if (id >= repo.size()) {
	  std::cerr << "Error. No." << id << " is not registerd in the object repository. " << repo.size() << std::endl;
	  valid = false;
	}
	if (id < repo.size() && repo[id] != 0) {
	  try {
	    GraphNode *objects = getNode(id);
	    if (objects == 0) {
	      std::cerr << "Error! Cannot get the node. " << id << std::endl;
	      valid = false;
	    }
#if defined(NGT_SHARED_MEMORY_ALLOCATOR)
	    for (GraphNode::iterator ri = objects->begin(repo.allocator);
		 ri != objects->end(repo.allocator); ++ri) {
#else
	    for (GraphNode::iterator ri = objects->begin();
		 ri != objects->end(); ++ri) {
#endif
#if defined(NGT_SHARED_MEMORY_ALLOCATOR)
	      for (GraphNode::iterator rj = objects->begin(repo.allocator) + std::distance(objects->begin(repo.allocator), ri);
		   rj != objects->end(repo.allocator); ++rj) {
		if ((*ri).id == (*rj).id && std::distance(objects->begin(repo.allocator), ri) != std::distance(objects->begin(repo.allocator), rj)) {
		  std::cerr << "Error! More than two identical objects! ID=" << (*rj).id << " idx=" 
		       << std::distance(objects->begin(repo.allocator), ri) << ":" << std::distance(objects->begin(repo.allocator), rj) 
		       << " disntace=" << (*ri).distance << ":" << (*rj).distance << std::endl;
#else
	      for (GraphNode::iterator rj = objects->begin() + std::distance(objects->begin(), ri);
		   rj != objects->end(); ++rj) {
		if ((*ri).id == (*rj).id && std::distance(objects->begin(), ri) != std::distance(objects->begin(), rj)) {
		  std::cerr << "Error! More than two identical objects! ID=" << (*rj).id << " idx=" 
		       << std::distance(objects->begin(), ri) << ":" << std::distance(objects->begin(), rj) 
		       << " disntace=" << (*ri).distance << ":" << (*rj).distance << std::endl;
#endif
		  valid = false;
	        }
	      }

	      if ((*ri).id == 0 || (*ri).id >= repo.size()) {
		std::cerr << "Error! Neighbor's ID of the node is out of range. ID=" << id << std::endl;
		valid = false;
	      } else if (repo[(*ri).id] == 0) {
		std::cerr << "Error! The neighbor ID of the node is invalid. ID=" << id << " Invalid ID=" << (*ri).id << std::endl;
		if (fr[(*ri).id] == 0) {
		  std::cerr <<  "The neighbor doesn't exist in the object repository as well. ID=" << (*ri).id << std::endl;
		} else {
		  std::cerr <<  "The neighbor exists in the object repository. ID=" << (*ri).id << std::endl;
		}
		valid = false;
	      }
	      if ((*ri).distance < 0.0) {
		std::cerr << "Error! Neighbor's distance is munus. ID=" << id << std::endl;
		valid = false;
	      }
	    }
	  } catch (Exception &err) {
	    std::cerr << "Error! Cannot get the node. " << id << " " << err.what() << std::endl;
	    valid = false;
	  }
	}
      }
      return valid;
    }

    static bool showStatisticsOfGraph(NGT::GraphIndex &outGraph, char mode = '-', size_t edgeSize = UINT_MAX)
    {
      long double distance = 0.0;
      size_t numberOfNodes = 0;
      size_t numberOfOutdegree = 0;
      size_t numberOfNodesWithoutEdges = 0;
      size_t maxNumberOfOutdegree = 0;
      size_t minNumberOfOutdegree = SIZE_MAX;
      std::vector<int64_t> indegreeCount;
      std::vector<size_t> outdegreeHistogram;
      std::vector<size_t> indegreeHistogram;
      std::vector<std::vector<float> > indegree;
      NGT::GraphRepository &graph = outGraph.repository;
      NGT::ObjectRepository &repo = outGraph.objectSpace->getRepository();
      indegreeCount.resize(graph.size(), 0);
      indegree.resize(graph.size());
      size_t removedObjectCount = 0;
      bool valid = true;
      for (size_t id = 1; id < graph.size(); id++) {
	if (repo[id] == 0) {
	  removedObjectCount++;
	  continue;
	}
	NGT::GraphNode *node = 0;
	try {
	  node = outGraph.getNode(id);
	} catch(NGT::Exception &err) {
	  std::cerr << "ngt info: Error. Cannot get the node. ID=" << id << ":" << err.what() << std::endl;
	  valid = false;
	  continue;
	}
	numberOfNodes++;
	if (numberOfNodes % 1000000 == 0) {
	  std::cerr << "Processed " << numberOfNodes << std::endl;
	}
	size_t esize = node->size() > edgeSize ? edgeSize : node->size();
	if (esize == 0) {
	  numberOfNodesWithoutEdges++;
	}
	if (esize > maxNumberOfOutdegree) {
	  maxNumberOfOutdegree = esize;
	}
	if (esize < minNumberOfOutdegree) {
	  minNumberOfOutdegree = esize;
	}
	if (outdegreeHistogram.size() <= esize) {
	  outdegreeHistogram.resize(esize + 1);
	}
	outdegreeHistogram[esize]++;
	if (mode == 'e') {
	  std::cout << id << "," << esize << ": ";
	}
	for (size_t i = 0; i < esize; i++) {
#if defined(NGT_SHARED_MEMORY_ALLOCATOR)
	  NGT::ObjectDistance &n = (*node).at(i, graph.allocator);
#else
	  NGT::ObjectDistance &n = (*node)[i];
#endif
	  if (n.id == 0) {
	    std::cerr << "ngt info: Warning. id is zero." << std::endl;
	    valid = false;
	  }
	  indegreeCount[n.id]++;
	  indegree[n.id].push_back(n.distance);
	  numberOfOutdegree++;
	  double d = n.distance;
	  if (mode == 'e') {
	    std::cout << n.id << ":" << d << " ";
	  }
	  distance += d;
	}
	if (mode == 'e') {
	  std::cout << std::endl;
	}
      }

      if (mode == 'a') {
	size_t count = 0;
	for (size_t id = 1; id < graph.size(); id++) {
	  if (repo[id] == 0) {
	    continue;
	  }
	  NGT::GraphNode *n = 0;
	  try {
	    n = outGraph.getNode(id);
	  } catch(NGT::Exception &err) {
	    continue;
	  }
	  NGT::GraphNode &node = *n;
	  for (size_t i = 0; i < node.size(); i++) {  
	    NGT::GraphNode *nn = 0;
	    try {
#if defined(NGT_SHARED_MEMORY_ALLOCATOR)
	      nn = outGraph.getNode(node.at(i, graph.allocator).id);
#else
	      nn = outGraph.getNode(node[i].id);
#endif
	    } catch(NGT::Exception &err) {
	      count++;
#if defined(NGT_SHARED_MEMORY_ALLOCATOR)
	      std::cerr << "Directed edge! " << id << "->" << node.at(i, graph.allocator).id << " no object. " 
		   << node.at(i, graph.allocator).id << std::endl; 
#else
	      std::cerr << "Directed edge! " << id << "->" << node[i].id << " no object. " << node[i].id << std::endl; 
#endif
	      continue;
	    }
	    NGT::GraphNode &nnode = *nn;
	    bool found = false;
	    for (size_t i = 0; i < nnode.size(); i++) {  
#if defined(NGT_SHARED_MEMORY_ALLOCATOR)
	      if (nnode.at(i, graph.allocator).id == id) {
#else
	      if (nnode[i].id == id) {
#endif
		found = true;
		break;
	      }
	    }
	    if (!found) {
#if defined(NGT_SHARED_MEMORY_ALLOCATOR)
	      std::cerr << "Directed edge! " << id << "->" << node.at(i, graph.allocator).id << " no edge. " 
		   << node.at(i, graph.allocator).id << "->" << id << std::endl; 
#else
	      std::cerr << "Directed edge! " << id << "->" << node[i].id << " no edge. " << node[i].id << "->" << id << std::endl; 
#endif
	      count++;
	    }
	  }
	}
	std::cerr << "# of not undirected edges=" << count << std::endl;
      }

      // calculate outdegree distance 10
      size_t d10count = 0;
      long double distance10 = 0.0;
      size_t d10SkipCount = 0;
      const size_t dcsize = 10;
      for (size_t id = 1; id < graph.size(); id++) {
	if (repo[id] == 0) {
	  continue;
	}
	NGT::GraphNode *n = 0;
	try {
	  n = outGraph.getNode(id);
	} catch(NGT::Exception &err) {
	  std::cerr << "ngt info: Warning. Cannot get the node. ID=" << id << ":" << err.what() << std::endl;
	  continue;
	}
	NGT::GraphNode &node = *n;
	if (node.size() < dcsize - 1) {
	  d10SkipCount++;
	  continue;
	}
	for (size_t i = 0; i < node.size(); i++) {  
	  if (i >= dcsize) {
	    break;
	  }
#if defined(NGT_SHARED_MEMORY_ALLOCATOR)
	  distance10 += node.at(i, graph.allocator).distance;
#else
	  distance10 += node[i].distance;
#endif
	  d10count++;
	}
      }
      distance10 /= (long double)d10count;

      // calculate indegree distance 10
      size_t ind10count = 0;
      long double indegreeDistance10 = 0.0;
      size_t ind10SkipCount = 0;
      for (size_t id = 1; id < indegree.size(); id++) {
	std::vector<float> &node = indegree[id];
	if (node.size() < dcsize - 1) {
	  ind10SkipCount++;
	  continue;
	}
	std::sort(node.begin(), node.end());
	for (size_t i = 0; i < node.size(); i++) {  
	  assert(i == 0 || node[i - 1] <= node[i]);
	  if (i >= dcsize) {
	    break;
	  }
	  indegreeDistance10 += node[i];
	  ind10count++;
	}
      }
      indegreeDistance10 /= (long double)ind10count;

      // calculate variance
      double averageNumberOfOutdegree = (double)numberOfOutdegree / (double)numberOfNodes;
      double sumOfSquareOfOutdegree = 0;
      double sumOfSquareOfIndegree = 0;
      for (size_t id = 1; id < graph.size(); id++) {
	if (repo[id] == 0) {
	  continue;
	}
	NGT::GraphNode *node = 0;
	try {
	  node = outGraph.getNode(id);
	} catch(NGT::Exception &err) {
	  std::cerr << "ngt info: Warning. Cannot get the node. ID=" << id << ":" << err.what() << std::endl;
	  continue;
	}
	size_t esize = node->size();
	sumOfSquareOfOutdegree += ((double)esize - averageNumberOfOutdegree) * ((double)esize - averageNumberOfOutdegree);
	sumOfSquareOfIndegree += ((double)indegreeCount[id] - averageNumberOfOutdegree) * ((double)indegreeCount[id] - averageNumberOfOutdegree);	
      }

      size_t numberOfNodesWithoutIndegree = 0;
      size_t maxNumberOfIndegree = 0;
      size_t minNumberOfIndegree = INT64_MAX;
      for (size_t id = 1; id < graph.size(); id++) {
	if (graph[id] == 0) {
	  continue;
	}
	if (indegreeCount[id] == 0) {
	  numberOfNodesWithoutIndegree++;
	  std::cerr << "Error! The node without incoming edges. " << id << std::endl;
	  valid = false;
	}
	if (indegreeCount[id] > static_cast<int>(maxNumberOfIndegree)) {
	  maxNumberOfIndegree = indegreeCount[id];
	}
	if (indegreeCount[id] < static_cast<int64_t>(minNumberOfIndegree)) {
	  minNumberOfIndegree = indegreeCount[id];
	}
	if (static_cast<int>(indegreeHistogram.size()) <= indegreeCount[id]) {
	  indegreeHistogram.resize(indegreeCount[id] + 1);
	}
	indegreeHistogram[indegreeCount[id]]++;
      }

      size_t count = 0;
      int medianOutdegree = -1;
      size_t modeOutdegree = 0;
      size_t max = 0;
      double c95 = 0.0;
      double c99 = 0.0;
      for (size_t i = 0; i < outdegreeHistogram.size(); i++) {
	count += outdegreeHistogram[i];
	if (medianOutdegree == -1 && count >= numberOfNodes / 2) {
	  medianOutdegree = i;
	}
	if (max < outdegreeHistogram[i]) {
	  max = outdegreeHistogram[i];
	  modeOutdegree = i;
	}
	if (count > numberOfNodes * 0.95) {
	  if (c95 == 0.0) {
	    c95 += i * (count - numberOfNodes * 0.95);
	  } else {
	    c95 += i * outdegreeHistogram[i];
	  }
	}
	if (count > numberOfNodes * 0.99) {
	  if (c99 == 0.0) {
	    c99 += i * (count - numberOfNodes * 0.99);
	  } else {
	    c99 += i * outdegreeHistogram[i];
	  }
	}
      }
      c95 /= (double)numberOfNodes * 0.05;
      c99 /= (double)numberOfNodes * 0.01;

      count = 0;
      int medianIndegree = -1;
      size_t modeIndegree = 0;
      max = 0;
      double c5 = 0.0;
      double c1 = 0.0;
      for (size_t i = 0; i < indegreeHistogram.size(); i++) {
	if (count < numberOfNodes * 0.05) {
	  if (count + indegreeHistogram[i] >= numberOfNodes * 0.05) {
	    c5 += i * (numberOfNodes * 0.05 - count);
	  } else {
	    c5 += i * indegreeHistogram[i];
	  }
	}
	if (count < numberOfNodes * 0.01) {
	  if (count + indegreeHistogram[i] >= numberOfNodes * 0.01) {
	    c1 += i * (numberOfNodes * 0.01 - count);
	  } else {
	    c1 += i * indegreeHistogram[i];
	  }
	}
	count += indegreeHistogram[i];
	if (medianIndegree == -1 && count >= numberOfNodes / 2) {
	  medianIndegree = i;
	}
	if (max < indegreeHistogram[i]) {
	  max = indegreeHistogram[i];
	  modeIndegree = i;
	}
      }
      c5 /= (double)numberOfNodes * 0.05;
      c1 /= (double)numberOfNodes * 0.01;

      std::cerr << "The size of object array=" << repo.size() << std::endl;
      std::cerr << "# of removed objects=" << removedObjectCount << "/" << repo.size() << std::endl;
      std::cerr << "# of nodes=" << numberOfNodes << std::endl;
      std::cerr << "# of edges=" << numberOfOutdegree << std::endl;
      std::cerr << "# of nodes without edges=" << numberOfNodesWithoutEdges << std::endl;
      std::cerr << "Max outdegree=" << maxNumberOfOutdegree << std::endl;
      std::cerr << "Min outdegree=" << minNumberOfOutdegree << std::endl;
      std::cerr << "Average number of edges=" << (double)numberOfOutdegree / (double)numberOfNodes << std::endl;
      std::cerr << "Average distance of edges=" << std::setprecision(10) << distance / (double)numberOfOutdegree << std::endl;
      std::cerr << "# of nodes where indegree is 0=" << numberOfNodesWithoutIndegree << std::endl;
      std::cerr << "Max indegree=" << maxNumberOfIndegree << std::endl;
      std::cerr << "Min indegree=" << minNumberOfIndegree << std::endl;
      std::cerr << "#-nodes,#-edges,#-no-indegree,avg-edges,avg-dist,max-out,min-out,v-out,max-in,min-in,v-in,med-out,"
	"med-in,mode-out,mode-in,c95,c5,o-distance(10),o-skip,i-distance(10),i-skip:" 
	   << numberOfNodes << ":" << numberOfOutdegree << ":" << numberOfNodesWithoutIndegree << ":" 
	   << std::setprecision(10) << (double)numberOfOutdegree / (double)numberOfNodes << ":"
	   << distance / (double)numberOfOutdegree << ":"
	   << maxNumberOfOutdegree << ":" << minNumberOfOutdegree << ":" << sumOfSquareOfOutdegree / (double)numberOfOutdegree<< ":"
	   << maxNumberOfIndegree << ":" << minNumberOfIndegree << ":" << sumOfSquareOfIndegree / (double)numberOfOutdegree << ":"
	   << medianOutdegree << ":" << medianIndegree << ":" << modeOutdegree << ":" << modeIndegree 
	   << ":" << c95 << ":" << c5 << ":" << c99 << ":" << c1 << ":" << distance10 << ":" << d10SkipCount << ":"
	   << indegreeDistance10 << ":" << ind10SkipCount << std::endl;
      if (mode == 'h') {
	std::cerr << "#\tout\tin" << std::endl;
	for (size_t i = 0; i < outdegreeHistogram.size() || i < indegreeHistogram.size(); i++) {
	  size_t out = outdegreeHistogram.size() <= i ? 0 : outdegreeHistogram[i];
	  size_t in = indegreeHistogram.size() <= i ? 0 : indegreeHistogram[i];
	  std::cerr << i << "\t" << out << "\t" << in << std::endl;
	}
      }
      return valid;
    }

    size_t getObjectRepositorySize() { return objectSpace->getRepository().size(); }

    size_t getSizeOfElement() { return objectSpace->getSizeOfElement(); }

    Object *allocateObject(const std::string &textLine, const std::string &sep) {
      return objectSpace->allocateNormalizedObject(textLine, sep);
    }
    Object *allocateObject(const std::vector<double> &obj) { 
      return objectSpace->allocateNormalizedObject(obj);
    }
    Object *allocateObject(const std::vector<float> &obj) { 
      return objectSpace->allocateNormalizedObject(obj);
    }
    Object *allocateObject(const std::vector<uint8_t> &obj) { 
      return objectSpace->allocateNormalizedObject(obj);
    }
    Object *allocateObject(const float *obj, size_t size) { 
      return objectSpace->allocateNormalizedObject(obj, size);
    }

    void deleteObject(Object *po) {
      return objectSpace->deleteObject(po);
    }

    ObjectSpace &getObjectSpace() { return *objectSpace; }

    void setupPrefetch(NGT::Property &prop);

    void setProperty(NGT::Property &prop) {
      setupPrefetch(prop);
      GraphIndex::property.set(prop);
      NeighborhoodGraph::property.set(prop);
      assert(property.dimension != 0);
      accuracyTable.set(property.accuracyTable);
    }

    void getProperty(NGT::Property &prop) {
      GraphIndex::property.get(prop);
      NeighborhoodGraph::property.get(prop);
    }

    NeighborhoodGraph::Property &getGraphProperty() { return NeighborhoodGraph::property; }

    virtual size_t getSharedMemorySize(std::ostream &os, SharedMemoryAllocator::GetMemorySizeType t) {
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
      size_t size = repository.getAllocator().getMemorySize(t);
#else
      size_t size = 0;
#endif
      os << "graph=" << size << std::endl;
      return size;
    }

    float getEpsilonFromExpectedAccuracy(double accuracy) { return accuracyTable.getEpsilon(accuracy); }

    protected:

    template <class REPOSITORY> void getSeedsFromGraph(REPOSITORY &repo, ObjectDistances &seeds) {
      if (repo.size() != 0) {
	size_t seedSize = repo.size() - 1 < (size_t)NeighborhoodGraph::property.seedSize ? 
	  repo.size() - 1 : (size_t)NeighborhoodGraph::property.seedSize;
	if (NeighborhoodGraph::property.seedType == NeighborhoodGraph::SeedTypeRandomNodes ||
	    NeighborhoodGraph::property.seedType == NeighborhoodGraph::SeedTypeNone) {
	  getRandomSeeds(repo, seeds, seedSize);
	} else if (NeighborhoodGraph::property.seedType == NeighborhoodGraph::SeedTypeFixedNodes) {
	  // To check speed using fixed seeds.
	  for (size_t i = 1; i <= seedSize; i++) {
	    ObjectDistance obj(i, 0.0);
	    seeds.push_back(obj);
	  }
	} else if (NeighborhoodGraph::property.seedType == NeighborhoodGraph::SeedTypeFirstNode) {
	  ObjectDistance obj(1, 0.0);
	  seeds.push_back(obj);
	} else {
	  getRandomSeeds(repo, seeds, seedSize);
	}
      }
    }

    // GraphIndex
    virtual void search(NGT::SearchContainer &sc, ObjectDistances &seeds) {
      if (sc.size == 0) {
	while (!sc.workingResult.empty()) sc.workingResult.pop();
	return;
      }
      if (seeds.size() == 0) {
#if defined(NGT_SHARED_MEMORY_ALLOCATOR) || !defined(NGT_GRAPH_READ_ONLY_GRAPH)
	getSeedsFromGraph(repository, seeds);
#else
	if (readOnly) {
	  getSeedsFromGraph(searchRepository, seeds);
	} else {
	  getSeedsFromGraph(repository, seeds);
	}
#endif
      }
      if (sc.expectedAccuracy > 0.0) {
	sc.setEpsilon(getEpsilonFromExpectedAccuracy(sc.expectedAccuracy));
      }

      NGT::SearchContainer so(sc);
      try {
	if (readOnly) {
#if defined(NGT_SHARED_MEMORY_ALLOCATOR) || !defined(NGT_GRAPH_READ_ONLY_GRAPH)
	  NeighborhoodGraph::search(so, seeds);
#else
	  (*searchUnupdatableGraph)(*this, so, seeds);
#endif
	} else {
	  NeighborhoodGraph::search(so, seeds);
	}
	sc.workingResult = std::move(so.workingResult);
	sc.distanceComputationCount = so.distanceComputationCount;
	sc.visitCount = so.visitCount;
      } catch(Exception &err) {
	std::cerr << err.what() << std::endl;
	Exception e(err);
	throw e;
      }
    }

    Index::Property			property;

    bool readOnly;
#ifdef NGT_GRAPH_READ_ONLY_GRAPH
    void (*searchUnupdatableGraph)(NGT::NeighborhoodGraph&, NGT::SearchContainer&, NGT::ObjectDistances&);
#endif

    Index::AccuracyTable		accuracyTable;
  };

  class GraphAndTreeIndex : public GraphIndex, public DVPTree {
  public:

#ifdef NGT_SHARED_MEMORY_ALLOCATOR
    GraphAndTreeIndex(const std::string &allocator, bool rdOnly = false):GraphIndex(allocator, false) {
      initialize(allocator, 0);
    }
    GraphAndTreeIndex(const std::string &allocator, NGT::Property &prop);
    void initialize(const std::string &allocator, size_t sharedMemorySize) {
      DVPTree::objectSpace = GraphIndex::objectSpace;
      DVPTree::open(allocator + "/tre", sharedMemorySize);
    }
#else
    GraphAndTreeIndex(const std::string &database, bool rdOnly = false) : GraphIndex(database, rdOnly) {
      GraphAndTreeIndex::loadIndex(database, rdOnly);
    }

    GraphAndTreeIndex(NGT::Property &prop) : GraphIndex(prop) {
      DVPTree::objectSpace = GraphIndex::objectSpace;
    }
#endif
    virtual ~GraphAndTreeIndex() {}

    void create() {}

#ifdef NGT_SHARED_MEMORY_ALLOCATOR
    void alignObjects()
    {
    }
#else 
    void alignObjects()
    {
      NGT::ObjectSpace &space = getObjectSpace();
      NGT::ObjectRepository &repo = space.getRepository();
      Object **object = repo.getPtr();
      std::vector<bool> exist(repo.size(), false);
      std::vector<NGT::Node::ID> leafNodeIDs;
      DVPTree::getAllLeafNodeIDs(leafNodeIDs);
      size_t objectCount = 0;
      for (size_t i = 0; i < leafNodeIDs.size(); i++) {
	ObjectDistances objects;
	DVPTree::getObjectIDsFromLeaf(leafNodeIDs[i], objects);
	for (size_t j = 0; j < objects.size(); j++) {
	  exist[objects[j].id] = true;
	  objectCount++;
	}
      }
      std::multimap<uint32_t, uint32_t> notexist; 
      if (objectCount != repo.size()) {
        for (size_t id = 1; id < exist.size(); id++) {
	  if (!exist[id]) {
            DVPTree::SearchContainer tso(*object[id]);
            tso.mode = DVPTree::SearchContainer::SearchLeaf;
            tso.radius = 0.0;
            tso.size = 1;
      	    try {
	      DVPTree::search(tso);
      	    } catch (Exception &err) {
	      std::stringstream msg;
	      msg << "GraphAndTreeIndex::getSeeds: Cannot search for tree.:" << err.what();
	      NGTThrowException(msg);
      	    }
	    notexist.insert(std::pair<uint32_t, uint32_t>(tso.nodeID.getID(), id));
	    objectCount++;
	  }
	}
      }
      assert(objectCount == repo.size() - 1);

      objectCount = 1;
      std::vector<std::pair<uint32_t, uint32_t> > order;  
      for (size_t i = 0; i < leafNodeIDs.size(); i++) {
	ObjectDistances objects;
	DVPTree::getObjectIDsFromLeaf(leafNodeIDs[i], objects);
	for (size_t j = 0; j < objects.size(); j++) {
	  order.push_back(std::pair<uint32_t, uint32_t>(objects[j].id, objectCount));
	  objectCount++;
	}
	auto nei = notexist.equal_range(leafNodeIDs[i].getID());
	for (auto ii = nei.first; ii != nei.second; ++ii) {
	  order.push_back(std::pair<uint32_t, uint32_t>((*ii).second, objectCount));
	  objectCount++;
	}
      }
      assert(objectCount == repo.size());
      Object *tmp = space.allocateObject();
      std::unordered_set<uint32_t> uncopiedObjects;
      for (size_t i = 1; i < repo.size(); i++) {
	uncopiedObjects.insert(i);
      }
      size_t copycount = 0;
      while (!uncopiedObjects.empty()) {
	size_t startID = *uncopiedObjects.begin();
	if (startID == order[startID - 1].first) {
	  uncopiedObjects.erase(startID);
	  copycount++;
	  continue;
	}
	size_t id = startID;
	space.copy(*tmp, *object[id]);
	uncopiedObjects.erase(id);    
	do {
	  space.copy(*object[id], *object[order[id - 1].first]);
	  copycount++;
	  id = order[id - 1].first;
	  uncopiedObjects.erase(id);
	} while (order[id - 1].first != startID);
	space.copy(*object[id], *tmp);
	copycount++;
      }
      space.deleteObject(tmp);

      assert(copycount == repo.size() - 1);

      sort(order.begin(), order.end());
      uncopiedObjects.clear();
      for (size_t i = 1; i < repo.size(); i++) {
	uncopiedObjects.insert(i);
      }
      copycount = 0;
      Object *tmpPtr;
      while (!uncopiedObjects.empty()) {
	size_t startID = *uncopiedObjects.begin();
	if (startID == order[startID - 1].second) {
	  uncopiedObjects.erase(startID);
	  copycount++;
	  continue;
	}
	size_t id = startID;
	tmpPtr = object[id];
	uncopiedObjects.erase(id);    
	do {
	  object[id] = object[order[id - 1].second];
	  copycount++;
	  id = order[id - 1].second;
	  uncopiedObjects.erase(id);
	} while (order[id - 1].second != startID);
	object[id] = tmpPtr;
	copycount++;
      }
      assert(copycount == repo.size() - 1);
    }
#endif // NGT_SHARED_MEMORY_ALLOCATOR

    void load(const std::string &ifile) {
      GraphIndex::load(ifile);
      DVPTree::objectSpace = GraphIndex::objectSpace;
    }

    void saveIndex(const std::string &ofile) {
      GraphIndex::saveIndex(ofile);
#ifndef NGT_SHARED_MEMORY_ALLOCATOR
      std::string fname = ofile + "/tre";
      std::ofstream ost(fname);
      if (!ost.is_open()) {
	std::stringstream msg;
	msg << "saveIndex:: Cannot open. " << fname;
	NGTThrowException(msg);
      }
      DVPTree::serialize(ost);
#endif
    }

    void loadIndex(const std::string &ifile, bool readOnly) {
      DVPTree::objectSpace = GraphIndex::objectSpace;
      std::ifstream ist(ifile + "/tre");
      DVPTree::deserialize(ist);
#ifdef NGT_GRAPH_READ_ONLY_GRAPH
      if (readOnly) {
	if (property.objectAlignment == NGT::Index::Property::ObjectAlignmentTrue) {
	  alignObjects();
	}
	GraphIndex::NeighborhoodGraph::loadSearchGraph(ifile);
      }
#endif
    }
    
    void exportIndex(const std::string &ofile) {
      GraphIndex::exportIndex(ofile);
      std::ofstream ost(ofile + "/tre");
      DVPTree::serializeAsText(ost);
    }

    void importIndex(const std::string &ifile) {
      std::string fname = ifile + "/tre";
      std::ifstream ist(fname);
      if (!ist.is_open()) {
	std::stringstream msg;
	msg << "importIndex:: Cannot open. " << fname;
	NGTThrowException(msg);
      }
      DVPTree::deserializeAsText(ist);
      GraphIndex::importIndex(ifile);
    }

    void remove(const ObjectID id, bool force = false) {
      Object *obj = 0;
      try {
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
	obj = GraphIndex::objectSpace->allocateObject(*GraphIndex::objectSpace->getRepository().get(id));
#else
	obj = GraphIndex::objectSpace->getRepository().get(id);
#endif
      } catch (Exception &err) {
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
	GraphIndex::objectSpace->deleteObject(obj);
#endif
	if (force) {
	  try {
	    DVPTree::removeNaively(id);
          } catch(...) {}
	  try {
	    GraphIndex::remove(id, force);
          } catch(...) {}
	  std::stringstream msg;
	  msg << err.what() << " Even though the object could not be found, the object could be removed from the tree and graph if it existed in them.";
	  NGTThrowException(msg);
        }
	throw err;
      }
      NGT::SearchContainer so(*obj);
      ObjectDistances results;
      so.setResults(&results);
      so.id = 0;
      so.size = 2;
      so.radius = 0.0;
      so.explorationCoefficient = 1.1;
      ObjectDistances	seeds;
      seeds.push_back(ObjectDistance(id, 0.0));
      GraphIndex::search(so, seeds);
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
      GraphIndex::objectSpace->deleteObject(obj);
#endif
      if (results.size() == 0) {
	NGTThrowException("No found the specified id");
      }
      if (results.size() == 1) {
	try {
	  DVPTree::remove(id);
	} catch(Exception &err) {
	  std::stringstream msg;
	  msg << "remove:: cannot remove from tree. id=" << id << " " << err.what();
	  NGTThrowException(msg);	
	}
      } else {
	ObjectID replaceID = id == results[0].id ? results[1].id : results[0].id;
	try {
	  DVPTree::replace(id, replaceID);
	} catch(Exception &err) {
	}
      }
      GraphIndex::remove(id, force);
    }

    void searchForNNGInsertion(Object &po, ObjectDistances &result) {
      NGT::SearchContainer sc(po);
      sc.setResults(&result);
      sc.size = NeighborhoodGraph::property.edgeSizeForCreation;
      sc.radius = FLT_MAX;
      sc.explorationCoefficient = NeighborhoodGraph::property.insertionRadiusCoefficient;
      sc.useAllNodesInLeaf = true;
      try {
	GraphAndTreeIndex::search(sc);
      } catch(Exception &err) {
	throw err;
      }
      if (static_cast<int>(result.size()) < NeighborhoodGraph::property.edgeSizeForCreation && 
	  result.size() < repository.size()) {
	if (sc.edgeSize != 0) {
	  try {
	    GraphAndTreeIndex::search(sc);
	  } catch(Exception &err) {
	    throw err;
	  }
	}
      }
    }

    void insert(ObjectID id) {
      ObjectRepository &fr = GraphIndex::objectSpace->getRepository();
      if (fr[id] == 0) {
	std::cerr << "GraphAndTreeIndex::insert empty " << id << std::endl;
	return;
      }
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
      Object &po = *GraphIndex::objectSpace->allocateObject(*fr[id]);
#else
      Object &po = *fr[id];
#endif
      ObjectDistances rs;
      if (NeighborhoodGraph::property.graphType == NeighborhoodGraph::GraphTypeANNG) {
	searchForNNGInsertion(po, rs);
      } else {
	searchForKNNGInsertion(po, id, rs);
      }

      GraphIndex::insertNode(id, rs);

      if (((rs.size() > 0) && (rs[0].distance != 0.0)) || rs.size() == 0) {
	DVPTree::InsertContainer tiobj(po, id);
	try {
	  DVPTree::insert(tiobj);
	} catch (Exception &err) {
	  std::cerr << "GraphAndTreeIndex::insert: Fatal error" << std::endl;
	  std::cerr << err.what() << std::endl;
	  return;
	}
      }
#ifdef NGT_SHARED_MEMORY_ALLOCATOR
      GraphIndex::objectSpace->deleteObject(&po);
#endif
    }

    void createIndex(size_t threadNumber);

    void createIndex(const std::vector<std::pair<NGT::Object*, size_t> > &objects, std::vector<InsertionResult> &ids,
		     double range, size_t threadNumber);

    void createTreeIndex();

    // GraphAndTreeIndex
    void getSeedsFromTree(NGT::SearchContainer &sc, ObjectDistances &seeds) {
      DVPTree::SearchContainer tso(sc.object);
      tso.mode = DVPTree::SearchContainer::SearchLeaf;
      tso.radius = 0.0;
      tso.size = 1;
      tso.distanceComputationCount = 0;
      tso.visitCount = 0;
      try {
	DVPTree::search(tso);
      } catch (Exception &err) {
	std::stringstream msg;
	msg << "GraphAndTreeIndex::getSeeds: Cannot search for tree.:" << err.what();
	NGTThrowException(msg);
      }

      try {
	DVPTree::getObjectIDsFromLeaf(tso.nodeID, seeds);
      } catch (Exception &err) {
	std::stringstream msg;
	msg << "GraphAndTreeIndex::getSeeds: Cannot get a leaf.:" << err.what();
	NGTThrowException(msg);
      }
      sc.distanceComputationCount += tso.distanceComputationCount;
      sc.visitCount += tso.visitCount;
      if (sc.useAllNodesInLeaf || NeighborhoodGraph::property.seedType == NeighborhoodGraph::SeedTypeAllLeafNodes) {
	return;
      }
      // if seedSize is zero, the result size of the query is used as seedSize.
      size_t seedSize = NeighborhoodGraph::property.seedSize == 0 ? sc.size : NeighborhoodGraph::property.seedSize;
      seedSize = seedSize > sc.size ? sc.size : seedSize;
      if (seeds.size() > seedSize) {
	srand(tso.nodeID.getID());
	// to accelerate thinning data.
	for (size_t i = seeds.size(); i > seedSize; i--) {
	  double random = ((double)rand() + 1.0) / ((double)RAND_MAX + 2.0);
	  size_t idx = floor(i * random);
	  seeds[idx] = seeds[i - 1];
	}
	seeds.resize(seedSize);
      } else if (seeds.size() < seedSize) {
	// A lack of the seeds is compansated by random seeds.
	//getRandomSeeds(seeds, seedSize);
      }
    }

    // GraphAndTreeIndex
    void search(NGT::SearchContainer &sc) {
      sc.distanceComputationCount = 0;
      sc.visitCount = 0;
      ObjectDistances	seeds;
      getSeedsFromTree(sc, seeds);
      GraphIndex::search(sc, seeds);
    }

    void search(NGT::SearchQuery &searchQuery) {
      Object *query = Index::allocateObject(searchQuery.getQuery(), searchQuery.getQueryType());
      try {
        NGT::SearchContainer sc(searchQuery, *query);
        sc.distanceComputationCount = 0;
        sc.visitCount = 0;
        ObjectDistances	seeds;
	getSeedsFromTree(sc, seeds);
	GraphIndex::search(sc, seeds);
      } catch(Exception &err) {
	deleteObject(query);
	throw err;
      }
      deleteObject(query);
    }

    size_t getSharedMemorySize(std::ostream &os, SharedMemoryAllocator::GetMemorySizeType t) {
      return GraphIndex::getSharedMemorySize(os, t) + DVPTree::getSharedMemorySize(os, t);
    }

    bool verify(std::vector<uint8_t> &status, bool info, char mode);

  };

  class Property : public Index::Property, public NeighborhoodGraph::Property {
  public:
    void setDefault() {
      Index::Property::setDefault();
      NeighborhoodGraph::Property::setDefault();
    }
    void clear() {
      Index::Property::clear();
      NeighborhoodGraph::Property::clear();
    }
    void set(NGT::Property &p) {
      Index::Property::set(p);
      NeighborhoodGraph::Property::set(p);
    }
    void load(const std::string &file) {
      NGT::PropertySet prop;
      prop.load(file + "/prf");
      Index::Property::importProperty(prop);
      NeighborhoodGraph::Property::importProperty(prop);
    }
    void importProperty(const std::string &file) {
      NGT::PropertySet prop;
      prop.load(file + "/prf");
      Index::Property::importProperty(prop);
      NeighborhoodGraph::Property::importProperty(prop);
    }
  };

} // namespace NGT


template<typename T>
size_t NGT::Index::append(std::vector<T> &object) 
{
  if (getObjectSpace().getRepository().size() == 0) {
    getObjectSpace().getRepository().initialize();
  }

  auto *o = getObjectSpace().getRepository().allocateNormalizedPersistentObject(object);
  getObjectSpace().getRepository().push_back(dynamic_cast<PersistentObject*>(o));
  size_t oid = getObjectSpace().getRepository().size() - 1;
  return oid;
}

template<typename T>
size_t NGT::Index::insert(std::vector<T> &object) 
{
  if (getObjectSpace().getRepository().size() == 0) {
    getObjectSpace().getRepository().initialize();
  }

  auto *o = getObjectSpace().getRepository().allocateNormalizedPersistentObject(object);
  size_t oid = getObjectSpace().getRepository().insert(dynamic_cast<PersistentObject*>(o));
  return oid;
}


