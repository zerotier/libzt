#!/usr/bin/env groovy

parallel 'centos7': {
    node('centos7') {
        try {
            checkout scm
            stage('Build centos7') {
                sh 'make linux_service_and_intercept SDK_LWIP=1 SDK_IPV4=1'
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
    }
}