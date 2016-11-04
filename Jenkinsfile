#!/usr/bin/env groovy

def changelog = getChangeLog currentBuild
slackSend "Building ${env.JOB_NAME} #${env.BUILD_NUMBER} \n Change Log: \n ${changelog}"

parallel 'centos7': {
    node('centos7') {

        // service (lwIP IPv4)
        try {
            checkout scm
            stage('linux_sdk_service lwIP IPv4') {
                sh 'make linux_sdk_service SDK_LWIP=1 SDK_IPV4=1'
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
            stage('linux_sdk_service lwIP IPv6') {
                sh 'make linux_sdk_service SDK_LWIP=1 SDK_IPV6=1'
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
            stage('Build centos7') {
                sh 'make linux_sdk_service SDK_PICOTCP=1 SDK_IPV4=1'
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
            stage('Build centos7') {
                sh 'make linux_sdk_service SDK_PICOTCP=1 SDK_IPV6=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on centos7 (<${env.BUILD_URL}|Open>)"
            throw err
        }

        // intercept
        try {
            checkout scm
            stage('Build centos7') {
                sh 'make linux_intercept SDK_LWIP=1 SDK_IPV4=1'
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

        // osx_service_and_intercept
        try {
            checkout scm
            stage('Build macOS') {
                sh 'make osx_service_and_intercept SDK_LWIP=1 SDK_IPV4=1'
            }
        }
        catch (err) {
            currentBuild.result = "FAILURE"
            slackSend color: '#ff0000', message: "${env.JOB_NAME} broken on macOS (<${env.BUILD_URL}|Open>)"
            throw err
        }

        // osx_app_framework
        try {
            checkout scm
            stage('Build macOS App Framework') {
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
            stage('Build iOS App Framework') {
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
