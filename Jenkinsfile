#!/usr/bin/env groovy

def changelog = getChangeLog currentBuild
slackSend "Building ${env.JOB_NAME} #${env.BUILD_NUMBER} \n Change Log: \n ${changelog}"

parallel 'centos7': {
    node('centos7') {

    // ------------------------------------------------------------------------------
    // ---------------------------- static library (Linux) --------------------------
    // ------------------------------------------------------------------------------

// NO_STACK

        try {
            checkout scm
            stage('linux static lib, no stack') {
                sh 'make static_lib NO_STACK=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
            throw err
        }


        try {
            checkout scm
            stage('linux static lib, no stack, ipv4') {
                sh 'make static_lib NO_STACK=1 LIBZT_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
            throw err
        }
        

        try {
            checkout scm
            stage('linux static lib, no stack, ipv6') {
                sh 'make static_lib NO_STACK=1 LIBZT_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
            throw err
        }


        try {
            checkout scm
            stage('linux static lib, no stack, ipv4, ipv6') {
                sh 'make static_lib NO_STACK=1 LIBZT_IPV4=1 LIBZT_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
            throw err
        }


// picoTCP

        try {
            checkout scm
            stage('linux static lib, picoTCP') {
                sh 'make static_lib STACK_PICO=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
            throw err
        }


        try {
            checkout scm
            stage('linux static lib, picoTCP, ipv4') {
                sh 'make static_lib STACK_PICO=1 LIBZT_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
            throw err
        }


        try {
            checkout scm
            stage('linux static lib, picoTCP, ipv6') {
                sh 'make static_lib STACK_PICO=1 LIBZT_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
            throw err
        }


        try {
            checkout scm
            stage('linux static lib, picoTCP, ipv4, ipv6') {
                sh 'make static_lib STACK_PICO=1 LIBZT_IPV4=1 LIBZT_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
            throw err
        }


// lwIP


        try {
            checkout scm
            stage('linux static lib lwIP') {
                sh 'make static_lib STACK_LWIP=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
            throw err
        }



        try {
            checkout scm
            stage('linux static lib lwIP, ipv4') {
                sh 'make static_lib STACK_LWIP=1 LIBZT_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
            throw err
        }



        try {
            checkout scm
            stage('linux static lib lwIP, ipv6') {
                sh 'make static_lib STACK_LWIP=1 LIBZT_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
            throw err
        }



        try {
            checkout scm
            stage('linux static lib lwIP, ipv4, ipv6') {
                sh 'make static_lib STACK_LWIP=1 LIBZT_IPV4=1 LIBZT_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
            throw err
        }

    // ------------------------------------------------------------------------------
    // ------------------------------ Unit tests (linux) ----------------------------
    // ------------------------------------------------------------------------------

        try {
            checkout scm
            stage('linux unit tests') {
                sh 'make tests'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on linux (<${env.BUILD_URL}|Open>)"
            throw err
        }

    }
}, 'macOS': {
    node('macOS') {

        unlockKeychainMac "~/Library/Keychains/login.keychain-db"

    // ------------------------------------------------------------------------------
    // -------------------------- Intercept Library (macOS) -------------------------
    // ------------------------------------------------------------------------------

    // TODO

    // ------------------------------------------------------------------------------
    // ---------------------------- App Frameworks (macOS) --------------------------
    // ------------------------------------------------------------------------------

    // TODO

    // ------------------------------------------------------------------------------
    // ----------------------------- static library (macOS) -------------------------
    // ------------------------------------------------------------------------------

// NO_STACK

        try {
            checkout scm
            stage('macOS static lib, no stack') {
                sh 'make static_lib NO_STACK=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }

        
        try {
            checkout scm
            stage('macOS static lib, no stack, ipv4') {
                sh 'make static_lib NO_STACK=1 LIBZT_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }
        

        try {
            checkout scm
            stage('macOS static lib, no stack, ipv6') {
                sh 'make static_lib NO_STACK=1 LIBZT_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }


        try {
            checkout scm
            stage('macOS static lib, no stack, ipv4, ipv6') {
                sh 'make static_lib NO_STACK=1 LIBZT_IPV4=1 LIBZT_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }

// picoTCP

        try {
            checkout scm
            stage('macOS static lib, picoTCP') {
                sh 'make static_lib STACK_PICO=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }


        try {
            checkout scm
            stage('macOS static lib, picoTCP, ipv4') {
                sh 'make static_lib STACK_PICO=1 LIBZT_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }


        try {
            checkout scm
            stage('macOS static lib, picoTCP, ipv6') {
                sh 'make static_lib STACK_PICO=1 LIBZT_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }


        try {
            checkout scm
            stage('macOS static lib, picoTCP, ipv4, ipv6') {
                sh 'make static_lib STACK_PICO=1 LIBZT_IPV4=1 LIBZT_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }


// lwIP


        try {
            checkout scm
            stage('macOS static lib, lwIP') {
                sh 'make static_lib STACK_LWIP=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }


        try {
            checkout scm
            stage('macOS static lib, lwIP, ipv4') {
                sh 'make static_lib STACK_LWIP=1 LIBZT_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }


        try {
            checkout scm
            stage('macOS static lib, lwIP, ipv6') {
                sh 'make static_lib STACK_LWIP=1 LIBZT_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }


        try {
            checkout scm
            stage('macOS static lib, lwIP, ipv4, ipv6') {
                sh 'make static_lib STACK_LWIP=1 LIBZT_IPV4=1 LIBZT_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }


    // ------------------------------------------------------------------------------
    // ------------------------------ Unit tests (macOS) ----------------------------
    // ------------------------------------------------------------------------------

        try {
            checkout scm
            stage('macOS unit tests') {
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
