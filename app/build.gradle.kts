plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.example.myapplication"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.example.myapplication"
        minSdk = 30
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++20"
                abiFilters("arm64-v8a")
            }
        }
        ndk {
            abiFilters.add("arm64-v8a")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            signingConfig = signingConfigs.getByName("debug")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    buildFeatures {
        prefab = true
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

// ========== 复制引擎资源到 assets ==========

// 获取引擎根目录（从 app 向上 6 级到 PrismaEngine 根目录）
val engineRoot = file("../../..").absoluteFile

// 复制 Android runtime 内置资源
tasks.register<Copy>("copyEngineRuntimeAssets") {
    description = "复制引擎 Android runtime 资源到 assets"
    group = "internal"

    from("$engineRoot/resources/runtime/android") {
        include("shaders/**")
        include("textures/**")
    }
    into("src/main/assets")
    // 保持目录结构
    eachFile {
        relativePath = RelativePath(true, *relativePath.segments)
    }
}

// 在预构建时执行复制
tasks.named("preBuild") {
    dependsOn("copyEngineRuntimeAssets")
}

dependencies {

    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.games.activity)
    testImplementation(libs.junit)
    androidTestImplementation(libs.ext.junit)
    androidTestImplementation(libs.espresso.core)
}