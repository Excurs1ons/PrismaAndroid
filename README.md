# PrismaAndroid

An Android application project that demonstrates native C++ integration with Prisma technology.

## Project Overview

This Android project showcases the integration of Prisma engine capabilities within an Android application using native C++ components. The project is configured with modern Android development practices and includes:

- **Native Development**: C++ integration via CMake
- **Target Architecture**: ARM64 (arm64-v8a)
- **Minimum SDK**: API 30 (Android 11)
- **Target SDK**: API 34 (Android 14)
- **Language**: Java/Kotlin with C++ native components

## Technical Specifications

### Build Configuration
- **Gradle Version**: Latest stable
- **Android Gradle Plugin**: 8.x
- **CMake Version**: 3.22.1
- **NDK Support**: Yes (ARM64 only)
- **Build Types**: Debug, Release (with minification enabled)

### Project Structure
```
PrismaAndroid/
├── app/
│   ├── src/main/
│   │   ├── cpp/          # Native C++ source code
│   │   ├── java/         # Java/Kotlin source code
│   │   └── res/          # Android resources
│   ├── build.gradle.kts  # App-level build configuration
│   └── proguard-rules.pro
├── gradle/               # Gradle wrapper and configuration
├── build.gradle.kts      # Project-level build configuration
├── gradle.properties     # Project properties
└── settings.gradle.kts   # Project settings
```

## Features

- Native C++ integration for high-performance operations
- Prisma engine integration capabilities
- Optimized for ARM64 architecture
- Release build with code shrinking and obfuscation
- Modern Android development practices

## Getting Started

### Prerequisites
- Android Studio Hedgehog | 2023.1.1 or later
- Android SDK API 34
- Android NDK for ARM64 architecture
- CMake 3.22.1 or later

### Installation
1. Clone this repository
2. Open in Android Studio
3. Sync project with Gradle files
4. Build and run on an ARM64 Android device or emulator

### Build Commands
```bash
# Debug build
./gradlew assembleDebug

# Release build
./gradlew assembleRelease

# Install debug APK
./gradlew installDebug

# Clean build
./gradlew clean
```

## Optimization Notes

This project has been optimized for performance as documented in `OPTIMIZATION_LOG.md`. Key optimizations include:
- ARM64-specific optimizations
- Native code performance tuning
- Release build configurations
- Memory management improvements

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly on ARM64 devices
5. Submit a pull request

## License

This project is proprietary and confidential. All rights reserved.

## Contact

For questions or support regarding this project, please contact the development team.

---

**Note**: This project is configured specifically for ARM64 architecture to ensure optimal performance with the Prisma engine integration.