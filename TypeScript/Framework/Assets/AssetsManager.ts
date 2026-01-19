import * as UE from 'ue';
import { ISingleton } from "../Interface/ISingleton";
import Misc from '../Misc/Misc';

/**
 * 资源管理器类，用于加载和缓存蓝图类和对象。
 * 
 * @implements {ISingleton<AssetsManager>}
 */
export default class AssetsManager implements ISingleton<AssetsManager> {

    private static instance: AssetsManager;
    private m_mapBPClass: Map<string, UE.Class> = new Map();
    private m_mapBPObject: Map<string, UE.Object> = new Map();

    private constructor() { }

    Initialize(): void {

    }

    Destroy(): void {
        for (let [key, value] of this.m_mapBPClass) {
            Misc.GetTSSubsys().RemoveClass(value);
        }
        this.m_mapBPClass.clear();

        for (let [key, value] of this.m_mapBPObject) {
            Misc.GetTSSubsys().RemoveObject(value);
        }
        this.m_mapBPObject.clear();
    }

    public static getInstance(): AssetsManager {
        if (!AssetsManager.instance) {
            AssetsManager.instance = new AssetsManager();
        }
        return AssetsManager.instance;
    }

    /**
     * 同步加载 BP 类。
     * 
     * @param {string} strPath - BP 类的路径。
     * @returns {UE.Class} BP 类。
     * @memberof AssetsManager
     */
    public LoadBPClass(strPath: string): UE.Class {
        if (this.m_mapBPClass.has(strPath)) {
            if (Misc.IsValid(this.m_mapBPClass.get(strPath))) {
                return this.m_mapBPClass.get(strPath);
            }
        }
        let bpClass = UE.Class.Load(strPath);
        this.CacheBPClass(strPath, bpClass);
        return bpClass;
    }

    /**
     * 异步加载 BP 类。
     * 
     * @param {string} strPath - BP 类的路径。
     * @param {(bpClass: UE.Class) => void} callback - 加载完成后的回调函数。
     * @memberof AssetsManager
     */
    public LoadBPClassAsync(strPath: string, callback: (bpClass: UE.Class) => void): void {
        if (this.m_mapBPClass.has(strPath)) {
            if (Misc.IsValid(this.m_mapBPClass.get(strPath))) {
                callback(this.m_mapBPClass.get(strPath));
                return;
            }
        }

        try {
            this.LoadBPClassAsyncImpl(strPath, callback);
        } catch (error) {
            console.log(error.toString());
        }
    }

    private async LoadBPClassAsyncImpl(strPath: string, callback: (bpClass: UE.Class) => void): Promise<void> {
        let bpClass = await Misc.AsyncLoad(strPath);
        this.CacheBPClass(strPath, bpClass);
        callback(bpClass);
    }

    private CacheBPClass(strPath: string, bpClass: UE.Class): void {
        if (bpClass != null) {
            this.m_mapBPClass.set(strPath, bpClass);
            Misc.GetTSSubsys().CacheClass(bpClass);
        }
        else {
            console.log("CacheBPClass failed, bpClass is null");
        }
    }

    /**
     * 同步加载 BP 对象。
     * 
     * @param {string} strPath - BP 对象的路径。
     * @returns {UE.Object} BP 对象。
     * @memberof AssetsManager
     */
    public LoadBPObject(strPath: string): UE.Object {
        if (this.m_mapBPObject.has(strPath)) {
            if (Misc.IsValid(this.m_mapBPObject.get(strPath))) {
                return this.m_mapBPObject.get(strPath);
            }
        }
        let bpObject = UE.Object.Load(strPath);
        this.CacheBPObject(strPath, bpObject);
        return bpObject;
    }

    /**
     * 异步加载 BP 对象。
     * 
     * @param {string} strPath - BP 对象的路径。
     * @param {(bpObject: UE.Object) => void} callback - 加载完成后的回调函数。
     * @memberof AssetsManager
     */
    public LoadBPObjectAsync(strPath: string, callback: (bpObject: UE.Object) => void): void {
        if (this.m_mapBPObject.has(strPath)) {
            if (Misc.IsValid(this.m_mapBPObject.get(strPath))) {
                callback(this.m_mapBPObject.get(strPath));
                return;
            }
        }

        try {
            this.LoadBPObjectAsyncImpl(strPath, callback);
        } catch (error) {
            console.log(error.toString());
        }
    }

    private async LoadBPObjectAsyncImpl(strPath: string, callback: (bpObject: UE.Object) => void): Promise<void> {
        let bpObject = await Misc.AsyncLoad(strPath);
        this.CacheBPObject(strPath, bpObject);
        callback(bpObject);
    }

    private CacheBPObject(strPath: string, bpObject: UE.Object): void {
        if (bpObject != null) {
            this.m_mapBPObject.set(strPath, bpObject);
            Misc.GetTSSubsys().CacheObject(bpObject);
        }
        else {
            console.log("CacheBPObject failed, bpObject is null");
        }
    }
}