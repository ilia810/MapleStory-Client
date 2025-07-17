# MapleStory Client Headless Testing Framework

## Overview

The headless testing framework allows automated testing of the MapleStory client without requiring a graphical interface. This enables programmatic testing of maps, mobs, UI assets, and other game components in a CI/CD pipeline or automated testing environment.

## Architecture

### Core Components

1. **TestFramework** - Base testing infrastructure
   - `TestCase` - Base class for all tests
   - `TestSuite` - Groups related tests together
   - `TestRunner` - Executes tests and reports results
   - Test registration system using macros

2. **HeadlessMode** - Runs client without graphics
   - Initializes game systems without window
   - Provides simulation methods for input
   - Manages update loop for tests
   - Optional graphics mode for debugging

3. **Test Categories**
   - `MapLoadTest` - Tests map loading functionality
   - `MobSpawnTest` - Tests mob spawning and management
   - `UIAssetTest` - Tests UI asset loading and validation

## Building the Tests

```bash
# Build with test support
build_headless_tests.bat

# Or manually with MSBuild
MSBuild.exe MapleStory.vcxproj /p:Configuration=Debug /p:Platform=x64
```

## Running Tests

### Command Line Options

```bash
# Run all tests
MapleStory.exe --test

# Run specific test suite
MapleStory.exe --test-suite MapLoading

# Run specific test case
MapleStory.exe --test-case MapLoading LoadHenesys

# List all available tests
MapleStory.exe --test-list

# Set custom timeout (milliseconds)
MapleStory.exe --test --test-timeout 60000

# Enable graphics during tests (for debugging)
MapleStory.exe --test --test-graphics

# Show help
MapleStory.exe --test-help
```

## Writing New Tests

### Basic Test Structure

```cpp
#include "../TestFramework.h"
#include "../HeadlessMode.h"

namespace ms {
namespace Testing {

TEST(SuiteName, TestName) {
    // Get headless mode instance
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    // Log test progress
    log("Testing something important");
    
    // Perform test actions
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    // Test assertions
    assertEqual(expected, actual, "Values should match");
    assert(condition, "Condition should be true");
    
    // Wait for conditions
    headless.waitForCondition([]() { 
        return someCondition(); 
    }, 5000);
}

} // namespace Testing
} // namespace ms
```

### Available Assertions

- `assert(condition, message)` - Basic assertion
- `assertEqual(expected, actual, message)` - Compare values
- `assertNotNull(pointer, message)` - Check pointer validity
- `fail(message)` - Explicitly fail test
- `skip(reason)` - Skip test with reason
- `log(message)` - Add log entry to test results

### HeadlessMode API

```cpp
// Input simulation
headless.simulateKeyPress(keycode);
headless.simulateKeyRelease(keycode);
headless.simulateMouseMove(x, y);
headless.simulateMouseClick(x, y, leftButton);

// Wait utilities
headless.waitForMapLoad(mapId, timeoutMs);
headless.waitForUIElement(elementName, timeoutMs);
headless.waitForCondition(lambda, timeoutMs);

// State queries
headless.isMapLoaded(mapId);
headless.isUIElementVisible(elementName);
headless.isConnected();

// Access game objects
Stage* stage = headless.getStage();
UI* ui = headless.getUI();
```

## Test Examples

### Map Loading Test
```cpp
TEST(MapLoading, LoadHenesys) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    Stage* stage = headless.getStage();
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    assertEqual(100000000, stage->get_mapid());
}
```

### Mob Spawning Test
```cpp
TEST(MobSpawning, SpawnSingleMob) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    Stage* stage = headless.getStage();
    
    stage->add_mob(oid, mobId, stance, fh, newSpawn, team, position);
    
    headless.waitForCondition([&stage, oid]() {
        return stage->get_mob(oid) != nullptr;
    }, 2000);
    
    assertNotNull(stage->get_mob(oid));
}
```

### UI Asset Test
```cpp
TEST(UIAssets, LoginUIAssets) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    UI* ui = headless.getUI();
    
    ui->emplace<UILogin>();
    headless.waitForUIElement("Login", 2000);
    
    nl::node loginNode = NxFiles::UI()["Login.img"];
    assert(loginNode.size() > 0);
}
```

## Integration with CI/CD

### GitHub Actions Example
```yaml
- name: Run Headless Tests
  run: |
    cd C:\HeavenClient\MapleStory-Client
    MapleStory.exe --test
```

### Exit Codes
- `0` - All tests passed
- `1` - One or more tests failed

## Debugging Failed Tests

1. **Enable Graphics Mode**
   ```bash
   MapleStory.exe --test --test-graphics
   ```

2. **Run Single Test**
   ```bash
   MapleStory.exe --test-case SuiteName TestName
   ```

3. **Check Test Logs**
   - Test results include detailed logs
   - Failed tests show error messages and stack traces

## Best Practices

1. **Test Isolation**
   - Each test should be independent
   - Clean up resources in tearDown()
   - Don't rely on test execution order

2. **Timeouts**
   - Use appropriate timeouts for async operations
   - Default timeout is 30 seconds per test
   - Adjust with --test-timeout for slow operations

3. **Assertions**
   - Use descriptive assertion messages
   - Test one thing per test case
   - Log important state changes

4. **Performance**
   - Run tests in headless mode by default
   - Only enable graphics when debugging
   - Use waitForCondition() instead of sleep()

## Extending the Framework

To add new test categories:

1. Create new test file in `Testing/Tests/`
2. Include `TestFramework.h` and `HeadlessMode.h`
3. Use TEST macro to define tests
4. Tests are automatically registered

To add new HeadlessMode features:

1. Update `HeadlessMode.h` with new methods
2. Implement in `HeadlessMode.cpp`
3. Consider thread safety for async operations

## Troubleshooting

### Tests Not Found
- Ensure test files are included in build
- Check TEST macro syntax
- Verify namespace is correct

### Timeout Issues
- Increase timeout with --test-timeout
- Check if condition can ever be met
- Verify game state updates properly

### Graphics Issues
- Headless mode disables graphics by default
- Use --test-graphics only for debugging
- MockGraphics class handles render calls

### Network Issues
- Tests run offline by default
- Mock network responses as needed
- Use HeadlessMode::isConnected() to check state