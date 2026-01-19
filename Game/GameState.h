class IGameState {
public:
    virtual void Update(float dt) = 0;
    virtual void HandleInput() = 0;
};

class BuildState : public IGameState {
    void Update(float dt) override {
        // Only run code related to placing buildings
        // Tint ghost mesh, check collisions with gold, etc.
    }
};

class PlayState : public IGameState {
    void Update(float dt) override {
        // Run player movement, squad logic, etc.
    }
};
