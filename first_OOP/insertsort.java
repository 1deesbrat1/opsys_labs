
public class insertsort {
    public static void insertSort(int arr[]) {

        for (int i = 1; i < arr.length; i++) {
            int key = arr[i];
            int j = i - 1;

            while (j >= 0 && arr[j] > key) {
                arr[j + 1] = arr[j];
                j--;
            }

            arr[j + 1] = key;
        }
    }

    public static void main(String[] args) {
        int arr[] = {5, 2, 7, 3, 1, 9};

        System.err.println("Before:");
        for (int num : arr) {
            System.out.print(num + " ");
        }

        System.out.println();
        System.out.println("After: ");
        insertSort(arr);

        for (int num : arr) {
            System.out.print(num + " ");
        }
    }
}
