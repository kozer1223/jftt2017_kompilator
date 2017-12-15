#ifndef LABELMANAGER_HH
#define LABELMANAGER_HH

#include <string.h>
#include <gmp.h>
#include <gmpxx.h>
#include <map>
#include <vector>
#include <set>

class LabelManager {
  std::set<std::string> labelSet;
  long long int lastLabelIndex;

public:
  LabelManager();
  ~LabelManager();

  std::string nextLabel();
  std::string nextLabel(std::string);

};

#endif
