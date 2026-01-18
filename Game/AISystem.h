#include "System.h"

class AISystem : public System {
public:
    AISystem();
    ~AISystem();

    void initialize();
    void update(float deltaTime);
    void shutdown();
};