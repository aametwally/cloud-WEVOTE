/// <reference types="mongoose" />
import { RepositoryBase } from './model';
import * as mongoose from 'mongoose';
import { IExperimentModel, IStatus } from './experiment';
export interface IRemoteAddress {
    host: string;
    port: number;
    relativePath: string;
}
export interface IWevoteSubmitEnsemble {
    jobID: string;
    resultsRoute: IRemoteAddress;
    reads: IWevoteClassification[];
    status: IStatus;
    score: number;
    penalty: number;
    minNumAgreed: number;
}
export interface IWevoteClassification extends mongoose.Document {
    seqId: string;
    votes: mongoose.Types.Array<number>;
    resolvedTaxon?: number;
    numToolsReported?: number;
    numToolsAgreed?: number;
    numToolsUsed?: number;
    score?: number;
}
export interface IWevoteClassificationPatch extends mongoose.Document {
    experiment: mongoose.Types.ObjectId;
    patch: mongoose.Types.Array<IWevoteClassification>;
    status: IStatus;
}
export declare const wevoteClassificationSchema: mongoose.Schema;
export declare class WevoteClassificationPatchModel {
    static schema: mongoose.Schema;
    private static _model;
    static repo: RepositoryBase<IWevoteClassificationPatch>;
    static isClassified: (headerLine: string, sep?: string) => boolean;
    static isHeaderLine: (line: string, sep?: string) => boolean;
    static extractAlgorithms: (headerLine: string, sep?: string) => string[];
    static parseUnclassifiedEnsemble: (file: string) => IWevoteClassification[];
    static makeWevoteSubmission: (experiment: IExperimentModel, cb: (wevoteSubmission: IWevoteSubmitEnsemble) => void) => void;
}
