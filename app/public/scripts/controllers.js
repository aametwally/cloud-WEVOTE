/**
 * Created by asem on 06/06/17.
 */
'use strict';
angular.module('wevoteApp')

    .controller('MainController', ['$scope', function ($scope) {
        $scope.showInput = false;
        $scope.error = false;
        $scope.message = "Loading ...";
    }])

    .controller('InputController', ['$scope', 'availableDatabaseFactory', 'algorithmsFactory', 'experimentFactory',
        function ($scope, availableDatabaseFactory, algorithmsFactory, experimentFactory) {
            $scope.inputForm = {};

            var readsSources = [{
                value: "client",
                label: "Upload reads file"
            },
            {
                value: "server",
                label: "Use simulated reads from the server"
            }
            ];

            var taxonomySources = [{
                value: "NCBI",
                label: "Use NCBI taxonomy database"
            },
            {
                value: "custom",
                label: "Upload custom taxonomy database"
            }
            ];

            $scope.readsSources = readsSources;
            $scope.taxonomySources = taxonomySources;


            var emptyInput = {
                user: "public",
                email: "",
                description: "",
                private: false,
                readsSource: readsSources[0].value,
                taxonomySource: taxonomySources[0].value,
                reads: {
                    name: "",
                    description: "",
                    onServer: true,
                    uri: "",
                    data: "",
                    size: "",
                    count: 0 
                },
                taxonomy: {
                    name: "",
                    description: "",
                    onServer: true,
                    uri: "",
                    data: "",
                    size: ""
                },
                config: {
                    minScore: 0,
                    minNumAgreed: 0,
                    penalty: 2,
                    algorithms: ""
                }

            };

            $scope.availableDatabase = {};
            $scope.supportedAlgorithms = {};
            $scope.availableDatabaseLoaded = false;
            $scope.supportedAlgorithmsLoaded = false;

            $scope.areDataLoaded = function () {
                return $scope.availableDatabaseLoaded &&
                    $scope.supportedAlgorithmsLoaded;
            };

            availableDatabaseFactory.getAvailableDatabase().query(
                function (response) {
                    $scope.availableDatabase = response;
                    $scope.availableDatabaseLoaded = true;
                    $scope.showInput = $scope.areDataLoaded();
                    console.log($scope.showInput);
                },
                function (response) {
                    $scope.error = true;
                    $scope.message = "Error: " + response.status + " " + response.statusText;
                });

            algorithmsFactory.getSupportedAlgorithms().query(
                function (response) {
                    $scope.supportedAlgorithms = response;
                    $scope.supportedAlgorithms.forEach(function (alg) {
                        alg.used = true;
                    });
                    $scope.supportedAlgorithmsLoaded = true;
                    $scope.showInput = $scope.areDataLoaded();
                    console.log($scope.showInput);
                },
                function (response) {
                    $scope.error = true;
                    $scope.message = "Error: " + response.status + " " + response.statusText;
                });

            $scope.usedAlgorithms = function () {
                return $scope.supportedAlgorithms.filter(function (alg) {
                    return alg.used;
                });
            };
            $scope.experiment = JSON.parse(JSON.stringify(emptyInput));

            $scope.noAlgorithmChosen = false;
            $scope.postExperiment = function () {
                console.log('postExperiment() invoked.');
                console.log($scope.experiment);
                $scope.experiment.config.algorithms = $scope.usedAlgorithms();
                $scope.noAlgorithmChosen = $scope.usedAlgorithms().length === 0;


                if (!$scope.noAlgorithmChosen) {
                    experimentFactory.submit($scope.experiment);
                    $scope.experiment = JSON.parse(JSON.stringify(emptyInput));

                    $scope.inputForm.form.$setPristine();
                    $scope.inputForm.form.$setUntouched();
                }
            };


            $scope.readsUploader = {};
            $scope.readsUploaderPostValidation = true;
            $scope.taxonomyUploader = {};
            $scope.taxonomyUploaderPostValidation = true;
        }
    ])

    .controller('ReadsUploaderController', ['$scope', 'fileUploaderFactory', function ($scope, fileUploaderFactory) {
        var datasetUploader = fileUploaderFactory.getFileUploader(
            'upload/reads', 'Drop reads file here', 'External dataset uploader');

        $scope.readsUploader = datasetUploader;
        $scope.uploader = datasetUploader;

        $scope.uploader.onSuccessItem = function (fileItem, response, status, headers) {
            // console.info('onSuccessItem', fileItem, response, status, headers);
            console.log('success', response, headers);
            $scope.experiment.reads.uri =
                JSON.parse(JSON.stringify(headers.filename));
            $scope.experiment.reads.count =
                parseInt(headers.readscount, 10);

        };

        $scope.uploader.onAfterAddingFile = function (fileItem) {
            console.info('onAfterAddingFile', fileItem);

            this.queue = [fileItem];

            $scope.experiment.reads.name =
                JSON.parse(JSON.stringify(fileItem.file.name));
            $scope.experiment.reads.size =
                JSON.parse(JSON.stringify(fileItem.file.size));

            setTimeout(function () {
                $("[data-toggle='tooltip']").tooltip({
                    trigger: 'hover'
                });
            }, 500);
        };
    }])

    .controller('TaxonomyUploaderController', ['$scope', 'fileUploaderFactory', function ($scope, fileUploaderFactory) {
        var taxonomyUploader = fileUploaderFactory.getFileUploader(
            'upload/taxonomy', 'Drop taxonomy file here', 'Custom taxonomy uploader');

        $scope.taxonomyUploader = taxonomyUploader;
        $scope.uploader = taxonomyUploader;

        $scope.uploader.onSuccessItem = function (fileItem, response, status, headers) {
            // console.info('onSuccessItem', fileItem, response, status, headers);
            console.log('success', response, headers);
            $scope.experiment.taxonomy.uri =
                JSON.parse(JSON.stringify(headers.filename));
        };

        $scope.uploader.onAfterAddingFile = function (fileItem) {
            console.info('onAfterAddingFile', fileItem);
            this.queue = [fileItem];

            $scope.experiment.taxonomy.name =
                JSON.parse(JSON.stringify(fileItem.file.name));
            $scope.experiment.taxonomy.size =
                JSON.parse(JSON.stringify(fileItem.file.size));

            setTimeout(function () {
                $("[data-toggle='tooltip']").tooltip({
                    trigger: 'hover'
                });
            }, 500);
        };
    }])

    .controller('HeaderController', ['$scope', function ($scope) {
        $("#loginButton").click(function () {
            $("#loginModal").modal('toggle');
        });
    }])

    .controller('ExperimentController', ['$scope', 'experimentFactory', function ($scope, experimentFactory) {
        $scope.experiments = {};
        $scope.showExperiments = false;
        $scope.experimentsError = false;
        $scope.experimentsMessage = "Loading ...";

        experimentFactory.getExperiments().query(
            function (response) {
                $scope.experiments = response;
                $scope.showExperiments = true;
                setTimeout(function () {
                    $("[data-toggle='tooltip']").tooltip({
                        trigger: 'hover',
                        placement: 'top'
                    });

                    $scope.experiments.forEach(function (exp) {
                        $('#' + exp._id).popover({
                            html: true,
                            trigger: "focus",
                            placement: "bottom",
                            content: function () {
                                return $('#data-' + exp._id).html();
                            }
                        });
                        $('#' + exp._id).click(function (e) {
                            // Special stuff to do when this link is clicked...

                            // Cancel the default action
                            e.preventDefault();
                        });

                        $('#' + 'desc-' + exp._id).popover({
                            html: true,
                            trigger: "focus",
                            placement: "bottom",
                            content: function () {
                                return $('#desc-data-' + exp._id).html();
                            }
                        });
                        $('#' + 'desc-' + exp._id).click(function (e) {
                            // Special stuff to do when this link is clicked...

                            // Cancel the default action
                            e.preventDefault();
                        });

                        $('#' + 'param-' + exp._id).popover({
                            html: true,
                            trigger: "focus",
                            placement: "bottom",
                            content: function () {
                                return $('#param-data-' + exp._id).html();
                            }
                        });
                        $('#' + 'param-' + exp._id).click(function (e) {
                            // Special stuff to do when this link is clicked...

                            // Cancel the default action
                            e.preventDefault();
                        });
                    });

                }, 1000);
            },
            function (response) {
                $scope.experimentsError = true;
                $scope.experimentsMessage = "Error: " + response.status + " " + response.statusText;
            });


    }])

    .controller('InfoController', ['$scope', function ($scope) {

    }])

    .controller('FeedbackController', ['$scope', function ($scope) {


    }])

    ;