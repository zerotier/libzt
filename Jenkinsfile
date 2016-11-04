#!/usr/bin/env groovy

def changelog = getChangeLog currentBuild
slackSend "Building ${env.JOB_NAME} #${env.BUILD_NUMBER} \n Change Log: \n ${changelog}"

parallel 'centos7': {
    node('centos7') {


        // ----- Network Stacks -----


        // (lwIP IPv4)
        try {
            checkout scm
            stage('lwIP IPv4') {
                sh 'make clean; make lwip SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }

        // (lwIP IPv6)
        try {
            checkout scm
            stage('lwIP IPv6') {
                sh 'make clean; make lwip SDK_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }

        // (picoTCP IPv4)
        try {
            checkout scm
            stage('picoTCP IPv4') {
                sh 'make clean; make pico SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }

        // (picoTCP IPv6)
        try {
            checkout scm
            stage('picoTCP IPv6') {
                sh 'make clean; make pico SDK_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }


        // ----- SDK Service Library -----


        // service (lwIP IPv4)
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

        // service (lwIP IPv6)
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

        // service (picoTCP IPv4)
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

        // service (picoTCP IPv6)
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


        // ----- Intercept Library -----


        // intercept
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
    }
}, 'macOS': {
    node('macOS') {

        unlockKeychainMac "~/Library/Keychains/login.keychain-db"

        // osx_service_and_intercept
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


        // ----- App Frameworks -----


        // osx_app_framework
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

        // ios_app_framework
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
    }
}

slackSend color: "#00ff00", message: "${env.JOB_NAME} #${env.BUILD_NUMBER} Complete (<${env.BUILD_URL}|Show More...>)"
