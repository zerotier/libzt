#!/usr/bin/env groovy

def changelog = getChangeLog currentBuild
slackSend "Building ${env.JOB_NAME} #${env.BUILD_NUMBER} \n Change Log: \n ${changelog}"

parallel 'centos7': {
    node('centos7') {

    // ------------------------------------------------------------------------------
    // --------------------------- Network Stacks (Linux) ---------------------------
    // ------------------------------------------------------------------------------

        try {
            checkout scm
            stage('linux lwIP IPv4') {
                sh 'make clean; make lwip SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }
        try {
            checkout scm
            stage('linux lwIP IPv6') {
                sh 'make clean; make lwip SDK_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }
        try {
            checkout scm
            stage('linux picoTCP IPv4') {
                sh 'make clean; make pico SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }
        try {
            checkout scm
            stage('linux picoTCP IPv6') {
                sh 'make clean; make pico SDK_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }

    // ------------------------------------------------------------------------------
    // ------------------------ SDK Service Library (Linux) -------------------------
    // ------------------------------------------------------------------------------

        try {
            checkout scm
            stage('linux_sdk_service (lwIP IPv4)') {
                sh 'make clean; make linux_sdk_service SDK_LWIP=1 SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }
        try {
            checkout scm
            stage('linux_sdk_service (lwIP IPv6)') {
                sh 'make clean; make linux_sdk_service SDK_LWIP=1 SDK_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }
        try {
            checkout scm
            stage('linux_sdk_service (picoTCP IPv4)') {
                sh 'make clean; make linux_sdk_service SDK_PICOTCP=1 SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }
        try {
            checkout scm
            stage('linux_sdk_service (picoTCP IPv6)') {
                sh 'make clean; make linux_sdk_service SDK_PICOTCP=1 SDK_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }


    // ------------------------------------------------------------------------------
    // ---------------------------- shared library (Linux) --------------------------
    // ------------------------------------------------------------------------------


        try {
            checkout scm
            stage('macOS shared lib') {
                sh 'make linux_shared_lib SDK_PICOTCP=1 SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }

        try {
            checkout scm
            stage('macOS shared lib') {
                sh 'make linux_shared_lib SDK_PICOTCP=1 SDK_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }

    // ------------------------------------------------------------------------------
    // --------------------------- Intercept Library (Linux) ------------------------
    // ------------------------------------------------------------------------------

        try {
            checkout scm
            stage('linux_intercept') {
                sh 'make clean; make linux_intercept'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }

    // ------------------------------------------------------------------------------
    // ------------------------------ Unit tests (linux) ----------------------------
    // ------------------------------------------------------------------------------

        try {
            checkout scm
            stage('linux tests') {
                sh 'make tests'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }

    }
}, 'macOS': {
    node('macOS') {

        unlockKeychainMac "~/Library/Keychains/login.keychain-db"

    // ------------------------------------------------------------------------------
    // --------------------------- Network Stacks (macOS) ---------------------------
    // ------------------------------------------------------------------------------

        try {
            checkout scm
            stage('macOS lwIP IPv4') {
                sh 'make clean; make lwip SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }
        try {
            checkout scm
            stage('macOS lwIP IPv6') {
                sh 'make clean; make lwip SDK_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }
        try {
            checkout scm
            stage('macOS picoTCP IPv4') {
                sh 'make clean; make pico SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }
        try {
            checkout scm
            stage('macOS picoTCP IPv6') {
                sh 'make clean; make pico SDK_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }


    // ------------------------------------------------------------------------------
    // -------------------------- Intercept Library (macOS) -------------------------
    // ------------------------------------------------------------------------------

        try {
            checkout scm
            stage('osx_intercept') {
                sh 'make osx_intercept'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }

    // ------------------------------------------------------------------------------
    // -------------------------- SDK Service Library (macOS) -----------------------
    // ------------------------------------------------------------------------------

        try {
            checkout scm
            stage('osx_service_and_intercept (lwIP IPv4)') {
                sh 'make osx_service_and_intercept SDK_LWIP=1 SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }
        try {
            checkout scm
            stage('osx_service_and_intercept (lwIP IPv6)') {
                sh 'make osx_service_and_intercept SDK_LWIP=1 SDK_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }
        try {
            checkout scm
            stage('osx_service_and_intercept (picoTCP IPv4)') {
                sh 'make osx_service_and_intercept SDK_PICOTCP=1 SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }

        try {
            checkout scm
            stage('osx_service_and_intercept (lwIP IPv4)') {
                sh 'make osx_service_and_intercept SDK_PICOTCP=1 SDK_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }

    // ------------------------------------------------------------------------------
    // ---------------------------- App Frameworks (macOS) --------------------------
    // ------------------------------------------------------------------------------

        try {
            checkout scm
            stage('macOS App Framework') {
                sh 'make osx_app_framework'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }
        try {
            checkout scm
            stage('iOS App Framework') {
                sh 'make ios_app_framework'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on iOS (<${env.BUILD_URL}|Open>)"
            throw err
        }
    // ------------------------------------------------------------------------------
    // ----------------------------- shared library (macOS) -------------------------
    // ------------------------------------------------------------------------------


        try {
            checkout scm
            stage('macOS shared lib') {
                sh 'make osx_shared_lib SDK_PICOTCP=1 SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }

        try {
            checkout scm
            stage('macOS shared lib') {
                sh 'make osx_shared_lib SDK_PICOTCP=1 SDK_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }


    // ------------------------------------------------------------------------------
    // --------------------------- Android JNI Lib (macOS) --------------------------
    // ------------------------------------------------------------------------------

        //try {
        //    checkout scm
        //    stage('macOS android_jni_lib') {
        //        sh 'make android_jni_lib SDK_LWIP=1 SDK_IPV4=1'
        //    }
        //}
        //catch (err) {
        //    currentBuild.result = "FAILURE"
        //    slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
        //    throw err
        //}


    // ------------------------------------------------------------------------------
    // ------------------------------ Unit tests (macOS) ----------------------------
    // ------------------------------------------------------------------------------

        try {
            checkout scm
            stage('macOS tests') {
                sh 'make tests'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }
    }
}

slackSend color: "#00ff00", message: "${env.JOB_NAME} #${env.BUILD_NUMBER} Complete (<${env.BUILD_URL}|Show More...>)"
