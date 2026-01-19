import { IDestroyable } from './IDestroyable';

/**
 * @interface IPoolObject
 * @extends IDestroyable
 * 
 * @description
 * 对象池对象接口，继承自IDestroyable接口。
 */
export interface IPoolObject extends IDestroyable {
    /**
     * @method reset
     * 重置对象的方法。
     */
    Reset(): void;

    /**
     * @method IsRecycled
     * 获取对象是否已回收。
     * 
     * @returns boolean - 如果对象已回收则返回true，否则返回false。
     */
    IsRecycled(): boolean;

    /**
     * @method Recycle
     * 回收对象的方法。
     */
    Recycle(): void;

}
